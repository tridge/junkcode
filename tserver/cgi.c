/* 
   some simple CGI helper routines
   Copyright (C) Andrew Tridgell 1997-2001
   
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

/* 
   inplace handling of + and % escapes in http variables 
*/
static void unescape(char *p)
{
	unsigned v;

	while (*p) {
		if (*p == '+') *p = ' ';
		if (*p == '%' && sscanf(p+1, "%02x", &v) == 1) {
			*p = (char)v;
			memcpy(p+1, p+3, strlen(p+3)+1);
		}
		p++;
	}
}

/*
  read one line from a file, allocating space as need be
  adjust length on return
*/
static char *grab_line(FILE *f, int *length)
{
	char *ret = NULL;
	int i = 0;
	int len = 0;

	while (*length) {
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
		(*length)--;

		if (c == EOF) {
			(*length) = 0;
			break;
		}
		
		if (c == '\r') continue;

		if (strchr("\n&", c)) break;

		ret[i++] = c;
	}
	

	ret[i] = 0;
	return ret;
}

/*
  add a name/value pair to the list of cgi variables 
*/
static void put(struct cgi_state *cgi, const char *name, const char *value)
{
	struct cgi_var *var;
	int len;

	if (!name || !value) return;
	var = malloc(sizeof(*var));
	var->next = cgi->variables;
	
	/* trim leading spaces */
	while (*name && (*name == '+' || *name == ' ')) name++;

	var->name = strdup(name);
	var->value = strdup(value);
	unescape(var->name);
	unescape(var->value);

	/* trim trailing spaces */
	len = strlen(var->value);
	while (len && isspace(var->value[len-1])) {
		var->value[len-1] = 0;
		len--;
	}

	cgi->variables = var;
}


/*
  load all the variables passed to the CGI program. May have multiple variables
  with the same name and the same or different values. 
*/
static void load_variables(struct cgi_state *cgi)
{
	char *line;
	char *p, *s, *tok;
	int len;
	FILE *f = stdin;

	len = cgi->content_length;

	if (len > 0 && cgi->request_post) {
		while (len && (line=grab_line(f, &len))) {
			p = strchr(line,'=');
			if (p) {
				*p = 0;
				put(cgi, line, p+1);
			}
			free(line);
		}
	}

	if ((s=cgi->query_string)) {
		char *pp;
		for (tok=strtok_r(s,"&;", &pp);tok;tok=strtok_r(NULL,"&;", &pp)) {
			p = strchr(tok,'=');
			if (p) {
				*p = 0;
				put(cgi, tok, p+1);
			}
		}
	}
}


/*
  find a variable passed via CGI
  Doesn't quite do what you think in the case of POST text variables, because
  if they exist they might have a value of "" or even " ", depending on the 
  browser. Also doesn't allow for variables[] containing multiple variables
  with the same name and the same or different values.
*/
static const char *get(struct cgi_state *cgi, const char *name)
{
	struct cgi_var *var;

	for (var = cgi->variables; var; var = var->next) {
		if (strcmp(var->name, name) == 0) {
			return var->value;
		}
	}
	return NULL;
}

/* set a variable in the cgi template from a cgi variable */
static void setvar(struct cgi_state *cgi, const char *name)
{
	cgi->tmpl->put(cgi->tmpl, name, cgi->get(cgi, name));
}


/*
  tell a browser about a fatal error in the http processing
*/
static void http_error(struct cgi_state *cgi, 
		       const char *err, const char *header, const char *info)
{
	if (!cgi->got_request) {
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
}

/*
  send a http header based on file extension
*/
static enum MIME_TYPE http_header(struct cgi_state *cgi, const char *filename)
{
	int i;
	static struct {
		char *pattern;
		char *mime_type;
		enum MIME_TYPE type;
	} mime_types[] = {
		{"*.gif",  "image/gif",  MIME_TYPE_IMAGE_GIF},
		{"*.jpg",  "image/jpeg", MIME_TYPE_IMAGE_JPEG},
		{"*.txt",  "text/plain", MIME_TYPE_TEXT_PLAIN},
		{"*.html", "text/html",  MIME_TYPE_TEXT_HTML},
		{NULL,     "data",       MIME_TYPE_UNKNOWN},
	};

	printf("HTTP/1.0 200 OK\r\nConnection: close\r\n");

	for (i=0; mime_types[i].pattern; i++) {
		if (fnmatch(mime_types[i].pattern, filename, 0) == 0) break;
	}
	printf("Content-Type: %s\r\n\r\n", mime_types[i].mime_type);
	return mime_types[i].type;
}


/*
  handle a file download
*/
static void download(struct cgi_state *cgi, const char *path)
{
	enum MIME_TYPE mtype;
	size_t size;
	void *m;

	mtype = cgi->http_header(cgi, path);

	if (mtype == MIME_TYPE_TEXT_HTML) {
		cgi->tmpl->process(cgi->tmpl, path);
		return;
	}

	m = map_file(path, &size);
	if (m) {
		fwrite(m, 1, size, stdout);
		unmap_file(m, size);
	}
}

/*
  read and parse the http request
 */
static int setup(struct cgi_state *cgi)
{
	char line[1024];
	char *url=NULL;
	char *p;

	/* we are a mini-web server. We need to read the request from stdin */
	while (fgets(line, sizeof(line)-1, stdin)) {
		if (line[0] == '\r' || line[0] == '\n') break;
		if (strncasecmp(line,"GET ", 4)==0) {
			cgi->got_request = 1;
			url = strdup(&line[4]);
		} else if (strncasecmp(line,"POST ", 5)==0) {
			cgi->got_request = 1;
			cgi->request_post = 1;
			url = strdup(&line[5]);
		} else if (strncasecmp(line,"PUT ", 4)==0) {
			cgi->got_request = 1;
			cgi->http_error(cgi, "400 Bad Request", "",
					"This server does not accept PUT requests");
			return -1;
		} else if (strncasecmp(line,"Content-Length: ", 16)==0) {
			cgi->content_length = atoi(&line[16]);
		}
		/* ignore all other requests! */
	}

	if (!url) {
		cgi->http_error(cgi, "400 Bad Request", "",
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
		cgi->query_string = p+1;
		*p = 0;
	}

	cgi->url = url;

	cgi->pathinfo = url;

	while (*cgi->pathinfo == '/') cgi->pathinfo++;

	return 0;
}

/*
  destroy a open cgi state
*/
static void destroy(struct cgi_state *cgi)
{
	cgi->tmpl->destroy(cgi->tmpl);

	while (cgi->variables) {
		struct cgi_var *var = cgi->variables;
		free(var->name);
		free(var->value);
		cgi->variables = cgi->variables->next;
		free(var);
	}

	free(cgi->url);
	memset(cgi, 0, sizeof(cgi));
	free(cgi);
}

static struct cgi_state cgi_base = {
	/* methods */
	setup,
	destroy,
	http_header,
	load_variables,
	get,
	setvar,
	http_error,
	download,
	
	/* rest are zero */
};

struct cgi_state *cgi_init(void)
{
	struct cgi_state *cgi;

	cgi = malloc(sizeof(*cgi));
	memcpy(cgi, &cgi_base, sizeof(*cgi));

	cgi->tmpl = template_init();

	return cgi;
}
