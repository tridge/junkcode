/* 
   some simple CGI helper routines
   Copyright (C) Andrew Tridgell 1997-1998
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include "includes.h"

#define MAX_VARIABLES 1000

struct var {
	char *name;
	char *value;
};

static struct var variables[MAX_VARIABLES];
static int num_variables;
static int content_length;
static int request_post;
static char *query_string;
static char *baseurl;
static char *pathinfo;
static BOOL got_request;

static void unescape(char *buf)
{
	char *p=buf;

	while ((p=strchr(p,'+')))
		*p = ' ';

	p = buf;

	while (p && *p && (p=strchr(p,'%'))) {
		int c1 = p[1];
		int c2 = p[2];

		if (c1 >= '0' && c1 <= '9')
			c1 = c1 - '0';
		else if (c1 >= 'A' && c1 <= 'F')
			c1 = 10 + c1 - 'A';
		else if (c1 >= 'a' && c1 <= 'f')
			c1 = 10 + c1 - 'a';
		else {p++; continue;}

		if (c2 >= '0' && c2 <= '9')
			c2 = c2 - '0';
		else if (c2 >= 'A' && c2 <= 'F')
			c2 = 10 + c2 - 'A';
		else if (c2 >= 'a' && c2 <= 'f')
			c2 = 10 + c2 - 'a';
		else {p++; continue;}
			
		*p = (c1<<4) | c2;

		memcpy(p+1, p+3, strlen(p+3)+1);
		p++;
	}
}


static char *grab_line(FILE *f, int *cl)
{
	char *ret = NULL;
	int i = 0;
	int len = 0;

	while ((*cl)) {
		int c;
	
		if (i == len) {
			char *ret2;
			if (len == 0) len = 1024;
			else len *= 2;
			ret2 = (char *)realloc(ret, len);
			if (!ret2) return ret;
			ret = ret2;
		}
	
		c = fgetc(f);
		(*cl)--;

		if (c == EOF) {
			(*cl) = 0;
			break;
		}
		
		if (c == '\r') continue;

		if (strchr("\n&", c)) break;

		ret[i++] = c;

	}
	

	ret[i] = 0;
	return ret;
}

/***************************************************************************
  load all the variables passed to the CGI program. May have multiple variables
  with the same name and the same or different values. Takes a file parameter
  for simulating CGI invocation eg loading saved preferences.
  ***************************************************************************/
void cgi_load_variables(void)
{
	static char *line;
	char *p, *s, *tok;
	int len;
	FILE *f = stdin;

	len = content_length;

	if (len > 0 && request_post) {
		while (len && (line=grab_line(f, &len))) {
			p = strchr(line,'=');
			if (!p) continue;
			
			*p = 0;
			
			variables[num_variables].name = strdup(line);
			variables[num_variables].value = strdup(p+1);

			SAFE_FREE(line);
			
			if (!variables[num_variables].name || 
			    !variables[num_variables].value)
				continue;

			unescape(variables[num_variables].value);
			unescape(variables[num_variables].name);

			num_variables++;
			if (num_variables == MAX_VARIABLES) break;
		}
	}

	fclose(stdin);
	open("/dev/null", O_RDWR);

	if ((s=query_string)) {
		for (tok=strtok(s,"&;");tok;tok=strtok(NULL,"&;")) {
			p = strchr(tok,'=');
			if (!p) continue;
			
			*p = 0;
			
			variables[num_variables].name = strdup(tok);
			variables[num_variables].value = strdup(p+1);

			if (!variables[num_variables].name || 
			    !variables[num_variables].value)
				continue;

			unescape(variables[num_variables].value);
			unescape(variables[num_variables].name);

			num_variables++;
			if (num_variables == MAX_VARIABLES) break;
		}
	}
}


/***************************************************************************
  find a variable passed via CGI
  Doesn't quite do what you think in the case of POST text variables, because
  if they exist they might have a value of "" or even " ", depending on the 
  browser. Also doesn't allow for variables[] containing multiple variables
  with the same name and the same or different values.
  ***************************************************************************/
char *cgi_variable(char *name)
{
	int i;

	for (i=0;i<num_variables;i++) {
		if (strcmp(variables[i].name, name) == 0)
			return variables[i].value;
	}
	return NULL;
}

/***************************************************************************
tell a browser about a fatal error in the http processing
  ***************************************************************************/
static void cgi_setup_error(char *err, char *header, char *info)
{
	if (!got_request) {
		/* damn browsers don't like getting cut off before they give a request */
		char line[1024];
		while (fgets(line, sizeof(line)-1, stdin)) {
			if (strncasecmp(line,"GET ", 4)==0 || 
			    strncasecmp(line,"POST ", 5)==0 ||
			    strncasecmp(line,"PUT ", 4)==0) {
				break;
			}
		}
	}

	printf("HTTP/1.0 %s\r\n%sConnection: close\r\nContent-Type: text/html\r\n\r\n<HTML><HEAD><TITLE>%s</TITLE></HEAD><BODY><H1>%s</H1>%s<p></BODY></HTML>\r\n\r\n", err, header, err, err, info);
	fclose(stdin);
	fclose(stdout);
	exit(0);
}

/* dump a mapped file, handing recursive include files for primitive 
   server side includes
*/
static void dump_includes(const char *m, size_t size)
{
	char *s;
	const char *p = m;
	char *fname;
	char *m2;
	size_t size2;

	while ((p = strstr(m, INCLUDE_TAG))) {
		const char *m0 = m;
		fwrite(m, 1, (p-m), stdout);
		m = p + strlen(INCLUDE_TAG);
		s = strstr(m, INCLUDE_TAG_END);
		if (!s) {
			fprintf(stderr,"No termination of include!\n");
			return;
		}
		fname = strndup(m, (s-m));
		m2 = map_file(fname, &size2);
		fprintf(stderr,"Including %s of size %d\n", fname, (int)size2);
		if (m2) dump_includes(m2, size2);
		unmap_file(m2, size2);
		m = s + strlen(INCLUDE_TAG_END);
		size -= (m - m0);
	}
	fwrite(m, 1, size, stdout);
	fflush(stdout);
}

/***************************************************************************
handle a file download
  ***************************************************************************/
void dump_file(const char *fname)
{
	const char *m;
	size_t size;
	m = map_file(fname, &size);
	if (!m) {
		fprintf(stderr,"Can't dump file %s\n", fname);
		return;
	}
	dump_includes(m, size);
	unmap_file(m, size);
}

/***************************************************************************
handle a file download
  ***************************************************************************/
void cgi_download(char *file)
{
	size_t size;
	int i;
	void *m;
	static struct {
		char *pattern;
		char *mime_type;
	} mime_types[] = {
		{"*.gif", "image/gif"},
		{"*.jpg", "image/jpeg"},
		{"*.txt", "text/plain"},
		{NULL, NULL}
	};

	/* sanitise the filename */
	for (i=0;file[i];i++) {
		if (!isalnum((int)file[i]) && !strchr("/.-_", file[i])) {
			cgi_setup_error("404 File Not Found","",
					"Illegal character in filename");
		}
	}

	m = map_file(file, &size);
	if (!m) {
		cgi_setup_error("404 File Not Found","",
				"The requested file was not found");
	}

	printf("HTTP/1.0 200 OK\r\n");
	for (i=0; mime_types[i].pattern; i++) {
		if (fnmatch(mime_types[i].pattern, file, 0) == 0) {
			printf("Content-Type: %s\r\n", mime_types[i].mime_type);
			printf("Content-Length: %d\r\n\r\n", (int)size);
			break;
		}
	}
	if (!mime_types[i].pattern) {
		printf("Content-Type: text/html\r\n\r\n");
		dump_includes(m, size);
	} else {
		fwrite(m, 1, size, stdout);
	}
	unmap_file(m, size);
	fflush(stdout);
}

/**
 * brief Setup the CGI framework.
 **/
void cgi_setup(void)
{
	char line[1024];
	char *url=NULL;
	char *p;
	struct stat st;

	/* we are a mini-web server. We need to read the request from stdin */
	while (fgets(line, sizeof(line)-1, stdin)) {
		if (line[0] == '\r' || line[0] == '\n') break;
		if (strncasecmp(line,"GET ", 4)==0) {
			got_request = True;
			url = strdup(&line[4]);
		} else if (strncasecmp(line,"POST ", 5)==0) {
			got_request = True;
			request_post = 1;
			url = strdup(&line[5]);
		} else if (strncasecmp(line,"PUT ", 4)==0) {
			got_request = True;
			cgi_setup_error("400 Bad Request", "",
					"This server does not accept PUT requests");
		} else if (strncasecmp(line,"Content-Length: ", 16)==0) {
			content_length = atoi(&line[16]);
		}
		/* ignore all other requests! */
	}

	if (!url) {
		cgi_setup_error("400 Bad Request", "",
				"You must specify a GET or POST request");
	}

	/* trim the URL */
	if ((p = strchr(url,' ')) || (p=strchr(url,'\t'))) {
		*p = 0;
	}
	while (*url && strchr("\r\n",url[strlen(url)-1])) {
		url[strlen(url)-1] = 0;
	}

	/* anything following a ? in the URL is part of the query string */
	if ((p=strchr(url,'?'))) {
		query_string = p+1;
		*p = 0;
	}

	while (*url == '/') url++;

	if (*url == 0) {
		url = "index.html";
	}

	if (strstr(url,"..")==0 && stat(url, &st) == 0) {
		cgi_download(url);
		exit(0);
	}

	printf("HTTP/1.0 200 OK\r\nConnection: close\r\n");
	baseurl = "";
	pathinfo = url;
	cgi_load_variables();
}
