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
static void put(struct template_state *tmpl, const char *name, const char *value)
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
	} else {
		printf("%s $%s %s", START_TAG, tag, END_TAG);
	}
}

/* process a inline shell script
   all current template variables are passed to the script in the environment
 */
static void process_shell(struct template_state *tmpl, const char *tag)
{
	pid_t pid;

	fflush(stdout);

	if ((pid=fork()) == 0) {
		struct template_var *var;
		for (var = tmpl->variables; var; var = var->next) {
			setenv(var->name, var->value, 1);
		} 
		dup2(1, 2);
		_exit(execl("/bin/sh", "sh", "-c", tag, NULL));
	}
	waitpid(pid, NULL, 0);
}

/*
  process a single tag
*/
static void process_tag(struct template_state *tmpl, const char *tag)
{
	while (isspace(*tag)) tag++;

	switch (*tag) {
	case '$':
		process_variable(tmpl, tag+1);
		break;
	case '!':
		process_shell(tmpl, tag+1);
		break;
	default:
		/* an include file */
		tmpl->process(tmpl, tag);
	}
}

/*
  process a template file
*/
static int process(struct template_state *tmpl, const char *filename)
{
	size_t size, remaining;
	char *m, *mp;
	char *contents;
	char *p, *s;

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

		fwrite(m, 1, (p-m), stdout);
		m = p + strlen(START_TAG);
		s = strstr(m, END_TAG);
		if (!s) {
			fprintf(stderr,"No termination of tag!\n");
			return -1;
		}
		len = (s-m);
		while (len && isspace(m[len-1])) len--;
		contents = strndup(m, len);
		process_tag(tmpl, contents);
		m = s + strlen(END_TAG);
		remaining -= (m - m0);
	}

	fwrite(m, 1, size, stdout);
	unmap_file(mp, size);
	fflush(stdout);
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
