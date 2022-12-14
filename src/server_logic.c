#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "boolean.h"
#include "error_handler.h"
#include "file_utils.h"
#include "http_response.h"
#include "http_request.h"
#include "http_status_codes.h"
#include "mcval.h"
#include "server_logic.h"


int handle_client(int client, char *webroot, bool verbose)
{
	char request[BUF_SIZE], req_method[BUF_SIZE], req_location[BUF_SIZE], req_dir[BUF_SIZE], req_file[BUF_SIZE];
	char req_params[BUF_SIZE], req_proto[BUF_SIZE];
	char response[BUF_SIZE], header[BUF_SIZE], body[BUF_SIZE];
	int bytes_read, status;	
		
	memset(request, '\0', BUF_SIZE);	
	memset(response, '\0', BUF_SIZE);

	while((bytes_read = read(client, request, BUF_SIZE-1)) > 0)
	{
		if(verbose)	
			fprintf(stdout, "Client request:\n%s\n", request);
			
		if(request[bytes_read - 1] == '\n')
			break;
	}
	
	if(bytes_read < 0)
	{
		notify(errno);
		status = errno_to_status(errno);
		send_header_only(client, PROTOCOL, status, header, response);	

		return FAILURE;
	}
	
	status = extract_req_params(request, req_method, req_location, req_params, req_proto, webroot);

	if(status != SUCCESS)
	{
		send_header_only(client, PROTOCOL, status, header, response);
		return FAILURE;
	}

	status = chop_reqested_location(req_location, req_dir, req_file);

	if(status != SUCCESS)
	{
		send_header_only(client, PROTOCOL, status, header, response);
                return FAILURE;
	}
	
	if(strncmp(req_method, "HEAD", strlen("HEAD")) == 0)
	{
		send_header_only(client, PROTOCOL, OK, header, response);
		return SUCCESS;
	}

	status = check_version_support(PROTOCOL, req_proto);

	if(status != SUCCESS)
	{
		send_header_only(client, PROTOCOL, status, header, response);
		return FAILURE;
	}

	status = check_method_support(req_method);

	if(status != SUCCESS)
	{
		send_header_only(client, PROTOCOL, status, header, response);
		return FAILURE;
	}

	status = is_subdirectory(webroot, req_dir);
	
	if(status != SUCCESS)
	{
		send_header_only(client, PROTOCOL, status, header, response);
		return FAILURE;
	}
	
	// 1. Read requested file 
	status = create_body(body, req_dir, req_file);

	if(status != SUCCESS)
	{
		send_header_only(client, PROTOCOL, status, header, response);
		return FAILURE;
	}

	// 2. Create header 
	create_header(PROTOCOL, OK, header, strlen(body));
	
	strncpy(response, header, strlen(header));
	strncat(response, body, strlen(body));
	
	write(client, response, BUF_SIZE);
      	
	return SUCCESS;
}
