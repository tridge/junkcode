/*
   Copyright (C) Andrew Tridgell 2002
   
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

/*
  automatic marshalling/unmarshalling system for C structures
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include <stdarg.h>
#include <errno.h>
#include "genparser.h"

struct parse_string {
	unsigned length;
	char *s;
};

static int addstr(struct parse_string *p, const char *fmt, ...)
{
	char *s = NULL;
	int n;
	va_list ap;
	va_start(ap, fmt);
	n = vasprintf(&s, fmt, ap);
	va_end(ap);
	p->s = realloc(p->s, p->length + n + 1);
	if (!p->s) {
		errno = ENOMEM;
		return -1;
	}
	if (n != 0) {
		memcpy(p->s + p->length, s, n);
	}
	p->length += n;
	p->s[p->length] = 0;

	if (s) free(s);
	return 0;
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

static int gen_dump_base(struct parse_string *p, 
			  enum parse_type type, 
			  const char *ptr)
{
	switch (type) {
	case T_STRING:
		return addstr(p, "{%s}", *(char **)(ptr));
	case T_FSTRING:
		return addstr(p, "{%s}", ptr);
	case T_TIME_T:
		return addstr(p, "%u", *(time_t *)(ptr));
	case T_UNSIGNED:
		return addstr(p, "%u", *(unsigned *)(ptr));
	case T_ENUM:
		return addstr(p, "%u", *(unsigned *)(ptr));
	case T_DOUBLE:
		return addstr(p, "%lg", *(double *)(ptr));
	case T_INT:
		return addstr(p, "%d", *(int *)(ptr));
	case T_LONG:
		return addstr(p, "%ld", *(long *)(ptr));
	case T_ULONG:
		return addstr(p, "%lu", *(unsigned long *)(ptr));
	case T_CHAR:
		return addstr(p, "%u", *(unsigned char *)(ptr));
	case T_FLOAT:
		return addstr(p, "%g", *(float *)(ptr));
	case T_STRUCT:
		return -1;
	}
	return 0;
}

static int gen_dump_one(struct parse_string *p, 
			 const struct parse_struct *pinfo,
			 const char *ptr,
			 unsigned indent)
{
	if (pinfo->type == T_STRUCT) {
		char *s = gen_dump(pinfo->pinfo, ptr, indent+1);
		if (!s) return -1;
		if (addstr(p, "{\n%s%s}", s, tabstr(indent)) != 0) {
			free(s);
			return -1;
		}
		free(s);
		return 0;
	}
	return gen_dump_base(p, pinfo->type, ptr);
}


static int gen_dump_array(struct parse_string *p,
			  const struct parse_struct *pinfo, 
			  const char *ptr,
			  int array_len,
			  int indent)
{
	int i, count=0;
	for (i=0;i<array_len;i++) {
		const char *p2 = ptr;
		unsigned size = pinfo->size;

		/* generic pointer dereference */
		if (pinfo->ptr_count) {
			p2 = *(const char **)ptr;
			size = sizeof(void *);
		}
		
		if (all_zero(p2, size)) {
			ptr += size;
			continue;
		}
		if (count == 0) {
			if (addstr(p, "%s%s = %u:", 
				   tabstr(indent),
				   pinfo->name, i) != 0) {
				return -1;
			}
		} else {
			if (addstr(p, ", %u:", i) != 0) {
				return -1;
			}
		}
		if (gen_dump_one(p, pinfo, p2, indent) != 0) {
			return -1;
		}
		ptr += size;
		count++;
	}
	if (count) {
		return addstr(p, "\n");
	}
	return 0;
}


static int find_var(const struct parse_struct *pinfo,
		    const char *data,
		    const char *var)
{
	int i;
	const char *ptr;

	for (i=0;pinfo[i].name;i++) {
		if (strcmp(pinfo[i].name, var) == 0) break;
	}
	if (!pinfo[i].name) return -1;

	ptr = data + pinfo[i].offset;

	switch (pinfo->type) {
	case T_UNSIGNED:
	case T_INT:
		return *(int *)ptr;
	case T_LONG:
	case T_ULONG:
		return *(long *)ptr;
	case T_CHAR:
		return *(char *)ptr;
	default:
		return -1;
	}
	return -1;
}

char *gen_dump(const struct parse_struct *pinfo, 
	       const char *data, 
	       unsigned indent)
{
	struct parse_string p;
	int i;
	
	p.length = 0;
	p.s = NULL;

	if (addstr(&p, "") != 0) {
		return NULL;
	}
	
	for (i=0;pinfo[i].name;i++) {
		const char *ptr = data + pinfo[i].offset;
		unsigned size = pinfo[i].size;

		if (pinfo[i].ptr_count) {
			size = sizeof(void *);
		}

		/* check for fixed length strings, assume null termination */
		if (pinfo[i].array_len != 0 && 
		    pinfo[i].ptr_count == 0 &&
		    pinfo[i].type == T_CHAR) {
			if (addstr(&p, "%s%s = ", tabstr(indent), pinfo[i].name) ||
			    gen_dump_base(&p, T_FSTRING, ptr) ||
			    addstr(&p, "\n")) {
				goto failed;
			}
			continue;
		}

		if (pinfo[i].array_len) {
			if (gen_dump_array(&p, &pinfo[i], ptr, 
					   pinfo[i].array_len, indent)) {
				goto failed;
			}
			continue;
		}

		if (pinfo[i].dynamic_len) {
			int len = find_var(pinfo, data, pinfo[i].dynamic_len);
			struct parse_struct p2 = pinfo[i];
			if (len < 0) {
				goto failed;
			}
			if (len > 0) {
				p2.ptr_count--;
				if (gen_dump_array(&p, &p2, *(char **)ptr, 
						   len, indent) != 0) {
					goto failed;
				}
			}
			continue;
		}

		if (all_zero(ptr, size)) continue;

		/* generic pointer dereference */
		if (pinfo[i].ptr_count) {
			ptr = *(const char **)ptr;
		}

		if (addstr(&p, "%s%s = ", tabstr(indent), pinfo[i].name) ||
		    gen_dump_one(&p, &pinfo[i], ptr, indent) ||
		    addstr(&p, "\n")) {
			goto failed;
		}
	}
	return p.s;

failed:
	free(p.s);
	return NULL;
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

static int gen_parse_base(const struct parse_struct *pinfo, 
			  char *ptr, 
			  const char *str)
{
	if (pinfo->ptr_count) {
		struct parse_struct p2 = *pinfo;
		*(void **)ptr = calloc(1, pinfo->ptr_count?sizeof(void *):pinfo->size);
		if (! *(void **)ptr) {
			return -1;
		}
		ptr = *(char **)ptr;
		p2.ptr_count--;
		return gen_parse_base(&p2, ptr, str);
	}

	switch (pinfo->type) {
	case T_STRING:
		*(char **)ptr = strdup(str);
		if (*(char **)ptr == NULL) {
			errno = ENOMEM;
			return -1;
		}
		break;
	case T_TIME_T:
	{
		unsigned v = 0;
		if (sscanf(str, "%u", &v) != 1) {
			errno = EINVAL;
			return -1;
		}
		*(time_t *)ptr = v;
		break;
	}
	case T_UNSIGNED:
	case T_ENUM:
	{
		unsigned v = 0;
		if (sscanf(str, "%u", &v) != 1) {
			errno = EINVAL;
			return -1;
		}
		*(unsigned *)ptr = v;
		break;
	}
	case T_INT:
	{
		int v = 0;
		if (sscanf(str, "%d", &v) != 1) {
			errno = EINVAL;
			return -1;
		}
		*(int *)ptr = v;
		break;
	}
	case T_LONG:
	{
		long v = 0;
		if (sscanf(str, "%ld", &v) != 1) {
			errno = EINVAL;
			return -1;
		}
		*(long *)ptr = v;
		break;
	}
	case T_ULONG:
	{
		unsigned long v = 0;
		if (sscanf(str, "%lu", &v) != 1) {
			errno = EINVAL;
			return -1;
		}
		*(unsigned long *)ptr = v;
		break;
	}
	case T_CHAR:
	{
		unsigned v = 0;
		if (sscanf(str, "%u", &v) != 1) {
			errno = EINVAL;
			return -1;
		}
		*(unsigned char *)ptr = v;
		break;
	}
	case T_FLOAT:
	{
		float v = 0;
		if (sscanf(str, "%g", &v) != 1) {
			errno = EINVAL;
			return -1;
		}
		*(float *)ptr = v;
		break;
	}
	case T_DOUBLE:
	{
		double v = 0;
		if (sscanf(str, "%lg", &v) != 1) {
			errno = EINVAL;
			return -1;
		}
		*(double *)ptr = v;
		break;
	}
	case T_STRUCT:
		return gen_parse(pinfo->pinfo, ptr, str);
	case T_FSTRING:
		return -1;
	}
	return 0;
}

static int gen_parse_array(const struct parse_struct *pinfo, 
			    char *ptr, 
			    const char *str,
			    int array_len)
{
	char *p, *p2;
	unsigned size = pinfo->size;

	/* special handling of fixed length strings */
	if (pinfo->array_len != 0 && 
	    pinfo->ptr_count == 0 &&
	    pinfo->type == T_CHAR) {
		strncpy(ptr, str, array_len);
		return 0;
	}

	if (pinfo->ptr_count) {
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

		if (gen_parse_base(pinfo, ptr + idx*size, p) != 0) {
			return -1;
		}

		if (done) break;
		str = p2+1;
	}

	return 0;
}

static int gen_parse_one(const struct parse_struct *pinfo, 
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
		return 0;
	}

	if (pinfo[i].array_len) {
		return gen_parse_array(&pinfo[i], data+pinfo[i].offset, 
				       str, pinfo[i].array_len);
	}

	if (pinfo[i].dynamic_len) {
		int len = find_var(pinfo, data, pinfo[i].dynamic_len);
		if (len < 0) {
			errno = EINVAL;
			return -1;
		}
		if (len > 0) {
			struct parse_struct p2 = pinfo[i];
			char *ptr = calloc(len, pinfo[i].size);
			if (!ptr) {
				errno = ENOMEM;
				return -1;
			}
			*((char **)(data + pinfo[i].offset)) = ptr;
			p2.ptr_count--;
			return gen_parse_array(&p2, ptr, str, len);
		}
		return 0;
	}

	return gen_parse_base(&pinfo[i], data + pinfo[i].offset, str);
}

int gen_parse(const struct parse_struct *pinfo, char *data, const char *s)
{
	char *str, *s0;
	
	s0 = strdup(s);
	str = s0;

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
			str = match_braces(value, '\n');
		}

		*str++ = 0;
		
		if (gen_parse_one(pinfo, name, data, value) != 0) {
			free(s0);
			return -1;
		}
	}

	free(s0);
	return 0;
}
