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

#define CONTENT_DISPOSITION "Content-Disposition:"
#define CONTENT_TYPE "Content-Type:"
#define MULTIPART_FORM_DATA "multipart/form-data"
#define CRLF "\r\n"

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
static char *grab_line(FILE *f, const char *terminator, int *length)
{
	int len = 1024;
	char *ret = malloc(len);
	int i = 0;
	int tlen = strlen(terminator);

	while (*length) {
		int c;
	
		if (i == len) {
			len *= 2;
			ret = realloc(ret, len);
		}
	
		c = fgetc(f);
		(*length)--;

		if (c == EOF) {
			(*length) = 0;
			break;
		}
		
		ret[i++] = c;

		if (memcmp(terminator, &ret[i-tlen], tlen) == 0) {
			i -= tlen;
			break;
		}
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
	char *cgi_name, *p;

	if (!name || !value) return;

	var = malloc(sizeof(*var));
	memset(var, 0, sizeof(*var));
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

	for (p=var->name; *p; p++) {
		if (!isalnum(*p) && !strchr("_-", *p)) {
			*p = '_';
		}
	}

	cgi->variables = var;
	asprintf(&cgi_name, "CGI_%s", var->name);
	cgi->tmpl->put(cgi->tmpl, cgi_name, var->value, NULL);
	free(cgi_name);
}


/*
  parse a url encoded form
*/
static void load_urlencoded(struct cgi_state *cgi)
{
	int len = cgi->content_length;
	char *line;
	char *p;
	FILE *f = stdin;

	while (len && (line=grab_line(f, "&", &len))) {
		p = strchr(line,'=');
		if (p) {
			*p = 0;
			put(cgi, line, p+1);
		}
		free(line);
	}
}

/*
  parse a single element of a multipart encoded form
  It's rather more complex than I would like :(
*/
static int load_one_part(struct cgi_state *cgi, FILE *f, int *len, char *boundary)
{
	char *line;
	char *name=NULL;
	char *content;
	char *filename=NULL;
	unsigned content_len=0, content_alloc=1024;
	unsigned boundary_len = strlen(boundary);
	int c;
	int raw_data = 0;

	while (*len && (line=grab_line(f, CRLF, len))) {
		if (*line == 0) break;
		if (strcmp(line,"--") == 0) return 1;
		if (strncasecmp(line, CONTENT_TYPE, 
				strlen(CONTENT_TYPE)) == 0) {
			raw_data = 1;
		}
		if (strncasecmp(line, CONTENT_DISPOSITION, 
				strlen(CONTENT_DISPOSITION)) == 0) {
			char *p = strstr(line,"; name=");
			if (!p) continue;
			p += 7;
			if (*p == '"') p++;
			name = strndup(p, strcspn(p, "\";"));
			p = strstr(line,"; filename=\"");
			if (p) {
				p += 12;
				filename = strndup(p, strcspn(p, "\";"));
			}
		}
	}
	
	content = malloc(content_alloc);
	
	while (*len && (c = fgetc(f)) != EOF) {
		(*len)--;
		if (content_len >= (content_alloc-1)) {
			content_alloc *= 2;
			content = realloc(content, content_alloc);
		}
		content[content_len++] = c;
		/* we keep grabbing content until we hit a boundary */
		if (memcmp(boundary, &content[content_len-boundary_len], 
			   boundary_len) == 0 &&
		    memcmp("--", &content[content_len-boundary_len-2], 2) == 0) {
			content_len -= boundary_len+4;
			if (name) {
				if (raw_data) {
					put(cgi, name, filename?filename:"");
					cgi->variables->content = content;
					cgi->variables->content_len = content_len;
				} else {
					content[content_len] = 0;
					put(cgi, name, content);
					free(name);
					free(content);
				}
			} else {
				free(content);
			}
			fgetc(f); fgetc(f);
			(*len) -= 2;
			return 0;
		}
	}

	if (filename) free(filename);

	return 1;
}

/*
  parse a multipart encoded form (for file upload)
  see rfc1867
*/
static void load_multipart(struct cgi_state *cgi)
{
	char *boundary;
	FILE *f = stdin;
	int len = cgi->content_length;
	char *line;

	if (!cgi->content_type) return;
	boundary = strstr(cgi->content_type, "boundary=");
	if (!boundary) return;
	boundary += 9;
	trim_tail(boundary, CRLF);
	line = grab_line(f, CRLF, &len);
	if (strncmp(line,"--", 2) != 0 || 
	    strncmp(line+2,boundary,strlen(boundary)) != 0) {
		fprintf(stderr,"Malformed multipart?\n");
		free(line);
		return;
	}

	if (strcmp(line+2+strlen(boundary), "--") == 0) {
		/* the end is only the beginning ... */
		free(line);
		return;
	}

	free(line);
	while (load_one_part(cgi, f, &len, boundary) == 0) ;
}

/*
  load all the variables passed to the CGI program. May have multiple variables
  with the same name and the same or different values. 
*/
static void load_variables(struct cgi_state *cgi)
{
	char *p, *s, *tok;

	if (cgi->content_length > 0 && cgi->request_post) {
		if (strncmp(cgi->content_type, MULTIPART_FORM_DATA, 
			    strlen(MULTIPART_FORM_DATA)) == 0) {
			load_multipart(cgi);
		} else {
			load_urlencoded(cgi);
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

/*
   return the content of a binary cgi variable (for file upload)
*/
static const char *get_content(struct cgi_state *cgi, const char *name, unsigned *size)
{
	struct cgi_var *var;

	for (var = cgi->variables; var; var = var->next) {
		if (strcmp(var->name, name) == 0) {
			*size = var->content_len;
			return var->content;
		}
	}
	return NULL;
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
		cgi->tmpl->process(cgi->tmpl, path, 1);
		return;
	}

	m = map_file(path, &size);
	if (m) {
		fwrite(m, 1, size, stdout);
		unmap_file(m, size);
	}
}


/* we're running under a web server as cgi */
static int setup_cgi(struct cgi_state *cgi)
{
	char *p;

	if ((p = getenv("CONTENT_LENGTH"))) {
		cgi->content_length = atoi(p);
	}
	if ((p = getenv("REQUEST_METHOD"))) {
		cgi->got_request = 1;
		if (strcasecmp(p, "POST") == 0) {
			cgi->request_post = 1;
		}
	}
	if ((p = getenv("QUERY_STRING"))) {
		cgi->query_string = strdup(p);
	}
	if ((p = getenv("SCRIPT_NAME"))) {
		cgi->url = strdup(p);
		cgi->pathinfo = cgi->url;
	}
	if ((p = getenv("CONTENT_TYPE"))) {
		cgi->content_type = strdup(p);
	}
	return 0;
}



/* we are a mini-web server. We need to read the request from stdin */
static int setup_standalone(struct cgi_state *cgi)
{
	char line[1024];
	char *url=NULL;
	char *p;

	while (fgets(line, sizeof(line)-1, stdin)) {
		trim_tail(line, CRLF);
		if (line[0] == 0) break;
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
		} else if (strncasecmp(line,"Content-Type: ", 14)==0) {
			cgi->content_type = strdup(&line[14]);
		}
		/* ignore all other requests! */
	}

	if (!url) {
		cgi->http_error(cgi, "400 Bad Request", "",
				"You must specify a GET or POST request");
		exit(1);
	}

	/* trim the URL */
	if ((p = strchr(url,' ')) || (p=strchr(url,'\t'))) {
		*p = 0;
	}

	/* anything following a ? in the URL is part of the query string */
	if ((p=strchr(url,'?'))) {
		cgi->query_string = p+1;
		*p = 0;
	}

	cgi->url = url;
	cgi->pathinfo = url;
	return 0;
}

/*
  read and parse the http request
 */
static int setup(struct cgi_state *cgi)
{
	int ret;

	if (getenv("GATEWAY_INTERFACE")) {
		ret = setup_cgi(cgi);
	} else {
		ret = setup_standalone(cgi);
	}

	while (cgi->pathinfo && *cgi->pathinfo == '/') cgi->pathinfo++;

	return ret;
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
	get_content,
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
