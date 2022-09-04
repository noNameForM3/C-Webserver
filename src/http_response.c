#include <string.h>
#include "http_response.h"
#include "gmt_date.h"

void create_status_line(char *protocol, char *status, char *msg)
{
	strcpy(msg, protocol);
	strcat(msg, " ");
	strcat(msg, status);
	strcat(msg, "\n");
}

void create_header_fields(char *data)
{
	char date[TOTAL_DATE_LEN]; 
	get_gmt_date_string(date);
	char s_name[] = "Server: webserv\n";
	char rel_loc[] = "Location: /\n";
	char conn[] = "Connection: close\n";
	char content_len[] = "Content-Length: 21\n";	// str-len body (if number is less than actual str it will be cut off)
	char content_type[] = "Content-Type: text/html; charset=iso-8859-1\n\n";

	strcpy(data, date);
	strcat(data, s_name);
	strcat(data, rel_loc);
	strcat(data, conn);
	strcat(data, content_len);
	strcat(data, content_type);
}