#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include <stdarg.h>
#include "struct.h"

enum parse_type {T_STRING, T_INT, T_UNSIGNED, T_CHAR,
		 T_FLOAT, T_DOUBLE, T_ENUM, T_STRUCT,
		 T_TIME_T, T_LONG};

#define FLAG_PTR 1

struct parse_struct {
	const char *name;
	enum parse_type type;
	unsigned flags;
	unsigned size;
	unsigned offset;
	unsigned array_len;
	struct parse_struct *pinfo;
};

#define T_PTR (1<<0)

struct parse_string {
	unsigned length;
	char *s;
};

static char *gen_dump(const struct parse_struct *pinfo, 
		      const char *data, 
		      unsigned indent);
static void gen_parse(const struct parse_struct *pinfo, char *data, const char *str0);

#include "parsers.c"

static void addstr(struct parse_string *p, const char *fmt, ...)
{
	char *s = NULL;
	int n;
	va_list ap;
	va_start(ap, fmt);
	n = vasprintf(&s, fmt, ap);
	va_end(ap);
	p->s = realloc(p->s, p->length + n + 1);
	memcpy(p->s + p->length, s, n+1);
	p->length += n;

	free(s);
}

static int all_zero(const char *ptr, unsigned size)
{
	int i;
	for (i=0;i<size;i++) {
		if (ptr[i]) return 0;
	}
	return 1;
}

static const char *tabstr(unsigned level)
{
	static char str[8];
	if (level > 7) level = 7;
	memset(str, '\t', level);
	str[level] = 0;
	return str;
}

static void gen_dump_base(struct parse_string *p, 
		     enum parse_type type, 
		     const char *ptr)
{
	switch (type) {
	case T_STRING:
		addstr(p, "{%s}", *(char **)(ptr));
		break;
	case T_TIME_T:
		addstr(p, "%u", *(time_t *)(ptr));
		break;
	case T_UNSIGNED:
		addstr(p, "%u", *(unsigned *)(ptr));
		break;
	case T_ENUM:
		addstr(p, "%u", *(unsigned *)(ptr));
		break;
	case T_DOUBLE:
		addstr(p, "%lg", *(double *)(ptr));
		break;
	case T_INT:
		addstr(p, "%d", *(int *)(ptr));
		break;
	case T_LONG:
		addstr(p, "%ld", *(long *)(ptr));
		break;
	case T_CHAR:
		addstr(p, "%u", *(unsigned char *)(ptr));
		break;
	case T_FLOAT:
		addstr(p, "%g", *(float *)(ptr));
		break;
	case T_STRUCT:
		break;
	}
}

static void gen_dump_one(struct parse_string *p, 
		    const struct parse_struct *pinfo,
		    const char *ptr,
		    unsigned indent)
{
	if (pinfo->type == T_STRUCT) {
		char *s = gen_dump(pinfo->pinfo, ptr, indent+1);
		addstr(p, "{\n%s%s}", s, tabstr(indent));
		free(s);
		return;
	}
	gen_dump_base(p, pinfo->type, ptr);
}


static char *gen_dump(const struct parse_struct *pinfo, 
		      const char *data, 
		      unsigned indent)
{
	struct parse_string p;
	int i;
	
	p.length = 0;
	p.s = NULL;
	
	for (i=0;pinfo[i].name;i++) {
		const char *ptr = data + pinfo[i].offset;
		unsigned size = pinfo[i].size;

		if (pinfo[i].flags & FLAG_PTR) {
			size = sizeof(void *);
		}

		if (pinfo[i].array_len) {
			int a, count=0;
			for (a=0;a<pinfo[i].array_len;a++) {
				const char *p2 = ptr;

				/* generic pointer dereference */
				if (pinfo[i].flags & FLAG_PTR) {
					p2 = *(const char **)ptr;
				}

				if (all_zero(p2, pinfo[i].size)) continue;
				if (count == 0) {
					addstr(&p, "%s%s = %u:", 
					       tabstr(indent),
					       pinfo[i].name, a);
				} else {
					addstr(&p, ", %u:", a);
				}
				gen_dump_one(&p, &pinfo[i], p2, indent);
				ptr += pinfo[i].size;
				count++;
			}
			if (count) {
				addstr(&p, "\n");
			}
			continue;
		}

		if (all_zero(ptr, size)) continue;

		/* generic pointer dereference */
		if (pinfo[i].flags & FLAG_PTR) {
			ptr = *(const char **)ptr;
		}

		addstr(&p, "%s%s = ", tabstr(indent), pinfo[i].name);
		gen_dump_one(&p, &pinfo[i], ptr, indent);
		addstr(&p, "\n");
	}
	return p.s;
}


static char *match_braces(char *s, char c)
{
	int depth = 0;
	while (*s) {
		if (*s == '}') {
			depth--;
		} else if (*s == '{') {
			depth++;
		}
		if (depth == 0 && *s == c) {
			return s;
		}
		s++;
	}
	return s;
}

static void gen_parse_base(const struct parse_struct *pinfo, 
			  char *data, 
			  const char *str)
{
	char *ptr = data + pinfo->offset;

	if (pinfo->flags & FLAG_PTR) {
		*(void **)ptr = calloc(1, pinfo->size);
		ptr = *(char **)ptr;
	}

	switch (pinfo->type) {
	case T_STRING:
		*(char **)ptr = strdup(str);
		break;
	case T_TIME_T:
	{
		unsigned v = 0;
		sscanf(str, "%u", &v);
		*(time_t *)ptr = v;
		break;
	}
	case T_UNSIGNED:
	case T_ENUM:
	{
		unsigned v = 0;
		sscanf(str, "%u", &v);
		*(unsigned *)ptr = v;
		break;
	}
	case T_INT:
	{
		int v = 0;
		sscanf(str, "%d", &v);
		*(int *)ptr = v;
		break;
	}
	case T_LONG:
	{
		long v = 0;
		sscanf(str, "%ld", &v);
		*(long *)ptr = v;
		break;
	}
	case T_CHAR:
	{
		unsigned v = 0;
		sscanf(str, "%u", &v);
		*(unsigned char *)ptr = v;
		break;
	}
	case T_FLOAT:
	{
		float v = 0;
		sscanf(str, "%g", &v);
		*(float *)ptr = v;
		break;
	}
	case T_DOUBLE:
	{
		double v = 0;
		sscanf(str, "%lg", &v);
		*(float *)ptr = v;
		break;
	}
	case T_STRUCT:
		gen_parse(pinfo->pinfo, ptr, str);
		break;
	}
}

static void gen_parse_array(const struct parse_struct *pinfo, 
			    char *data, 
			    const char *str)
{
	char *p, *p2;
	unsigned size = pinfo->size;

	if (pinfo->flags & FLAG_PTR) {
		size = sizeof(void *);
	}

	while (*str) {
		unsigned idx;
		int done;

		idx = atoi(str);
		p = strchr(str,':');
		if (!p) break;
		p++;
		p2 = match_braces(p, ',');
		done = (*p2 != ',');
		*p2 = 0;

		if (*p == '{') {
			p++;
			p[strlen(p)-1] = 0;
		}

		gen_parse_base(pinfo, data + idx*pinfo->size, p);

		if (done) break;
		str = p2+1;
	}
}

static void gen_parse_one(const struct parse_struct *pinfo, 
			  const char *name, 
			  char *data, 
			  const char *str)
{
	int i;
	for (i=0;pinfo[i].name;i++) {
		if (strcmp(pinfo[i].name, name) == 0) {
			break;
		}
	}
	if (pinfo[i].name == NULL) {
		return;
	}

	if (pinfo[i].array_len) {
		gen_parse_array(&pinfo[i], data, str);
		return;
	}

	gen_parse_base(&pinfo[i], data, str);
}

static void gen_parse(const struct parse_struct *pinfo, char *data, const char *str0)
{
	char *str;

	str = strdup(str0);

	while (*str) {
		char *p;
		char *name;
		char *value;

		/* skip leading whitespace */
		while (isspace(*str)) str++;

		p = strchr(str, '=');
		if (!p) break;
		value = p+1;
		while (p > str && isspace(*(p-1))) {
			p--;
		}
		*p = 0;
		name = str;

		while (isspace(*value)) value++;

		if (*value == '{') {
			str = match_braces(value, '}');
			value++;
		} else {
			str = strchr(value, '\n');
		}

		*str++ = 0;
		
		gen_parse_one(pinfo, name, data, value);
	}
}



int main(int argc, char *argv[])
{
	struct player player;

	memset(&player, 0, sizeof(player));

	gen_parse(pinfo_player, (char *)&player, argv[1]);
	
	printf("%s", dump_player(&player));

	return 0;
}
