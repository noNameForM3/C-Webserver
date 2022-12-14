#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "boolean.h"
#include "error_handler.h"
#include "server_logic.h"

#define BACKLOG 500		// max. number of pending connections
#define PORT 10000

extern int errno;

static const struct option long_options[] = 
{
	{ "port",	required_argument, 	0, 'p' },
	{ "webroot",	required_argument,	0, 'w' },
	{ "help", 	no_argument,		0, 'h' },
	{ 0, 0, 0, 0 }
};

char *help_msg = "Usage: ./webserv [ -p port_number ]\n"
	     "\t\tCommand summary:\n"
	     "\t\t\t-h\t\t\t\tShow this text\n"
	     "\t\t\t-p <port>\t\t\tSpecify port number (1 - 65535) for remote connections\n"
	     "\t\t\t-w <absolute path>\t\tSpecify web root directory\n" 
	     "\n\t\tCommand summary (long):\n"
	     "\t\t\t--help\t\t\t\tShow this text\n"
	     "\t\t\t--port <port>\t\t\tSpecify port number (1 - 65535) for remote connections\n"
	     "\t\t\t--webroot <absolute path>\tSpecify web root directory\n";

int main(int argc, char **argv)
{
	int port = -1;	// to check if user specified port, if not port == -1
	char webroot[PATH_MAX];

	memset(webroot, '\0', PATH_MAX);

	while(TRUE)
	{
		int option_index = 0;
		int c = getopt_long(argc, argv, "p:w:h", long_options, &option_index);

		if(c == -1)
			break;

		switch(c) 
		{
			case 'p':
				port = strtol(optarg, NULL, 10);
				
				if(errno != 0)
				{
					notify(errno);
					printf("Defaulting to port %d.\n", PORT);
					port = PORT;
					errno = 0;
				}

				if(port < 1 || port > 65535)
				{
					printf("Invalid port number. Defaulting to port %d.\n", PORT);
					port = PORT;
				}	
			
				break;
			case 'w':
				strncpy(webroot, optarg, PATH_MAX);

				if(chdir(webroot) == -1)		// change current working directory
					terminate(errno);

				break;
			case 'h':
				printf("%s", help_msg);
				exit(EXIT_SUCCESS);
				break;
			case '?':
				exit(EXIT_FAILURE);
				break;
		}
	}

	if(webroot[0] == '\0')
	{
		if(getcwd(webroot, PATH_MAX) == NULL)
			terminate(errno);
	}
	
	int serverFd, clientFd;
	int status;
	struct sockaddr_in6 serv_addr, cli_addr;
	socklen_t addr_len;
	char cli_addr_str[INET6_ADDRSTRLEN];
	bool verbose = true; 			// Add cmd line option to disable verbose?
	pid_t pid;

	
	serverFd = socket(AF_INET6, SOCK_STREAM, 0);
	
	if(serverFd == -1)
	{
		terminate(errno);
	}

	memset(&serv_addr, 0, sizeof(struct sockaddr_in6));
	serv_addr.sin6_family = AF_INET6;
	serv_addr.sin6_addr = in6addr_any;
       	serv_addr.sin6_port = htons( (port == -1) ? PORT : port);	

	status = bind(serverFd, (struct sockaddr*) &serv_addr, sizeof(struct sockaddr_in6));
	if(status == -1)
	{
		close(serverFd);
		terminate(errno);
	}
 
	status = listen(serverFd, BACKLOG);
	
	if(status == -1)
	{
		close(serverFd);
		terminate(errno);
	}

	printf("Web root directory: \t\t%s\n", webroot);
	printf("Server started on port: \t%d\n", (port == -1) ? PORT : port);
	
	while(TRUE)
	{
		addr_len = sizeof(struct sockaddr_in6);
		clientFd = accept(serverFd, (struct sockaddr*) &cli_addr, &addr_len);

		if(clientFd == -1)
		{
			close(serverFd);
			terminate(errno);
		}

		if(inet_ntop(AF_INET6, &cli_addr.sin6_addr, cli_addr_str,
					INET6_ADDRSTRLEN) == NULL)
		{
			printf("Accepted new connection.\n");
		} else {
		
			printf("Accepted new connection from: %s:%d\n", 
					cli_addr_str, ntohs(cli_addr.sin6_port));
		}
			
		pid = fork();
		
		if(pid == -1) 
		{
			terminate(errno);
		}
		
		else if (pid == 0) 
		{
			close(serverFd);
			handle_client(clientFd, webroot, verbose);
			close(clientFd);
			printf("Connection closed.\n");
			exit(EXIT_SUCCESS);
		}

		else 
		{
			close(clientFd);
		}	
	}

	exit(EXIT_SUCCESS);
}
