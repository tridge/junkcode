/* 
   some simple CGI helper routines
   Copyright (C) Andrew Tridgell 1997-2002
   
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

struct cgi_var {
	struct cgi_var *next;
	char *name;
	char *value;
	char *content;
	unsigned content_len;
};

struct cgi_state {
	/* methods */
	int (*setup)(struct cgi_state *);
	void (*destroy)(struct cgi_state *);
	enum MIME_TYPE (*http_header)(struct cgi_state *, const char *);
	void (*load_variables)(struct cgi_state *);
	const char *(*get)(struct cgi_state *, const char *);
	const char *(*get_content)(struct cgi_state *, const char *, unsigned *size);
	void (*http_error)(struct cgi_state *cgi, 
			   const char *err, const char *header, const char *info);
	void (*download)(struct cgi_state *cgi, const char *path);

	/* data */
	struct cgi_var *variables;
	struct template_state *tmpl;
	char *content_type;
	int content_length;
	int request_post;
	char *query_string;
	char *pathinfo;
	char *url;
	int got_request;
};


enum MIME_TYPE {MIME_TYPE_IMAGE_GIF, 
		MIME_TYPE_IMAGE_JPEG,		
		MIME_TYPE_TEXT_PLAIN,
		MIME_TYPE_TEXT_HTML,
		MIME_TYPE_UNKNOWN};

/* prototypes */
struct cgi_state *cgi_init(void);

