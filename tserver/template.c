/* 
   some simple html template routines
   Copyright (C) Andrew Tridgell 2001
   
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

static void process_tag(struct template_state *tmpl, const char *tag);

/* 
   fetch a variable from the template variables list 
*/
static struct template_var *find_var(struct template_state *tmpl, const char *name)
{
	struct template_var *var;

	for (var = tmpl->variables; var; var = var->next) {
		if (strcmp(var->name, name) == 0) {
			return var;
		}
	}
	return NULL;
}

/*
  add a name/value pair to the list of template variables 
*/
static void put(struct template_state *tmpl, const char *name, 
		const char *value, template_fn fn)
{
	struct template_var *var;
	if (!name || !value) return;

	var = find_var(tmpl, name);
	if (var) {
		free(var->value);
	} else {
		var = malloc(sizeof(*var));
		var->next = tmpl->variables;
		tmpl->variables = var;
		var->name = strdup(name);
		var->function = fn;
	}
	var->value = strdup(value);
}

/* fetch a variable from the template variables list */
static const char *get(struct template_state *tmpl, const char *name)
{
	struct template_var *var;

	var = find_var(tmpl, name);
	if (var) return var->value;

	return NULL;
}


/* process a template variable */
static void process_variable(struct template_state *tmpl, const char *tag)
{
	const char *v = tmpl->get(tmpl, tag);
	if (v) {
		printf("%s", v);
	}
}

/* process a template variable with quote escaping */
static void process_escaped_variable(struct template_state *tmpl, const char *tag)
{
	const char *v = tmpl->get(tmpl, tag);
	while (v && *v) {
		if (*v == '"') {
			printf("&quot;");
		} else {
			fputc(*v, stdout);
		}
		v++;
	}
}

/* process a inline shell script
   all current template variables are passed to the script in the environment
   recursively processes any tags in the output of the script
 */
static void process_shell(struct template_state *tmpl, const char *tag)
{
	pid_t pid;

	fflush(stdout);

	if ((pid=fork()) == 0) {
		struct template_var *var;
		FILE *p;
		char *tag1=NULL;
		int c, taglen=0;

		for (var = tmpl->variables; var; var = var->next) {
			setenv(var->name, var->value, 1);
		} 
		dup2(1, 2);
		p = popen(tag, "r");
		if (!p) {
			printf("Failed to execute script\n");
			exit(1);
		}
		while ((c = fgetc(p)) != EOF) {
			int depth;

			tag1 = realloc(tag1, taglen+2);
			tag1[taglen++] = c; tag1[taglen] = 0;
			if (strncmp(tag1, START_TAG, taglen) != 0) {
				fputs(tag1, stdout);
				free(tag1); tag1 = NULL; taglen = 0;
				continue;
			}
			if (strcmp(tag1, START_TAG) != 0) continue;

			depth = 1;
			/* keep going until END_TAG */
			while (depth && (c = fgetc(p)) != EOF) {
				tag1 = realloc(tag1, taglen+2);
				tag1[taglen++] = c; tag1[taglen] = 0;
				if (strcmp(tag1+taglen-strlen(END_TAG), 
					   END_TAG) == 0) {
					depth--;
				}
				if (strcmp(tag1+taglen-strlen(START_TAG), 
					   START_TAG) == 0) {
					depth++;
				}
			}
			if (depth) continue;

			tag1[taglen-strlen(END_TAG)] = 0;
			process_tag(tmpl, tag1+strlen(START_TAG));
			free(tag1); tag1 = NULL; taglen = 0;
		}
		if (tag1) {
			fputs(tag1, stdout);
			free(tag1);
		}
		fflush(stdout);
		fclose(p);
		exit(1);
	}
	waitpid(pid, NULL, 0);
}

/* process a call into a C function setup with put_function() */
static void process_c_call(struct template_state *tmpl, const char *tag)
{
	struct template_var *var;
	char *name, *args, *p, *tok;
	char **argv;
	int argc=0;

	if (!(p=strchr(tag, '('))) return;

	name = strndup(tag, strcspn(tag, "("));

	var = find_var(tmpl, name);
	if (!var || !var->function) {
		free(name);
		return;
	}

	args = strndup(p+1, strcspn(p+1, ")"));

	argv = malloc(sizeof(char *));
	for (tok = strtok_r(args, ",", &p); tok; tok = strtok_r(NULL, ",", &p)) {
		argv = realloc(argv, (argc+2)*sizeof(char *));
		while (isspace(*tok)) tok++;
		trim_tail(tok, " \t\r\n");
		argv[argc++] = tok;
	}
	argv[argc] = NULL;

	var->function(tmpl, name, var->value, argc, argv);
	free(args);
	free(name);
}

/*
  process a single tag
*/
static void process_tag(struct template_state *tmpl, const char *tag)
{
	char *tag2;

	while (isspace(*tag)) tag++;

	tag2 = strdup(tag);
	trim_tail(tag2, " \t\n\r");

	switch (*tag2) {
	case '$':
		process_variable(tmpl, tag2+1);
		break;
	case '%':
		process_escaped_variable(tmpl, tag2+1);
		break;
	case '!':
		process_shell(tmpl, tag2+1);
		break;
	case '@':
		process_c_call(tmpl, tag2+1);
		break;
	default:
		/* an include file */
		tmpl->process(tmpl, tag2);
	}
	free(tag2);

	fflush(stdout);
}

/*
  process a template file
*/
static int process(struct template_state *tmpl, const char *filename)
{
	size_t size, remaining;
	char *m, *mp;
	char *p, *s;

	setvbuf(stdout, NULL, _IONBF, 0);

	mp = map_file(filename, &size);
	if (!mp) {
		fprintf(stderr,"Failed to map %s (%s)\n", 
			filename, strerror(errno));
		return -1;
	}

	remaining = size;
	m = mp;

	/* tags look like {{ TAG }} 
	   where TAG can be of several forms
	*/
	while (remaining && (p = strstr(m, START_TAG))) {
		const char *m0 = m;
		int len;
		char *contents, *s2, *m2;
		int depth=1;

		fwrite(m, 1, (p-m), stdout);
		m = p + strlen(START_TAG);
		m2 = m;
		while (depth) {
			s2 = strstr(m2, START_TAG);
			s = strstr(m2, END_TAG);
			if (!s) break;
			if (s2 && s2 < s) {
				depth++;
				m2 = s2 + strlen(START_TAG);
			} else {
				depth--;
				m2 = s + strlen(END_TAG);
			}
		}
		if (!s || depth) {
			fprintf(stderr,"No termination of tag!\n");
			return -1;
		}
		len = (s-m);
		while (len && isspace(m[len-1])) len--;
		contents = strndup(m, len);
		process_tag(tmpl, contents);
		free(contents);
		m = s + strlen(END_TAG);
		remaining -= (m - m0);
	}

	if (remaining > 0) {
		fwrite(m, 1, remaining, stdout);
	}
	fflush(stdout);
	unmap_file(mp, size);
	return 0;
}

/*
  destroy a open template
*/
static void destroy(struct template_state *tmpl)
{
	while (tmpl->variables) {
		struct template_var *var = tmpl->variables;
		free(var->name);
		free(var->value);
		tmpl->variables = tmpl->variables->next;
		free(var);
	}

	memset(tmpl, 0, sizeof(tmpl));
	free(tmpl);
}


static struct template_state template_base = {
	/* methods */
	process,
	put,
	get,
	destroy
	
	/* rest are zero */
};

struct template_state *template_init(void)
{
	struct template_state *tmpl;

	tmpl = x_malloc(sizeof(*tmpl));
	memcpy(tmpl, &template_base, sizeof(*tmpl));

	return tmpl;
}
