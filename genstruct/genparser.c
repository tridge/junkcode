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

#if STANDALONE
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
#else
#include "includes.h"
#endif

struct parse_string {
	unsigned allocated;
	unsigned length;
	char *s;
};

static int all_zero(const char *ptr, unsigned size)
{
	int i;
	if (!ptr) return 1;
	for (i=0;i<size;i++) {
		if (ptr[i]) return 0;
	}
	return 1;
}

static char *encode_bytes(const char *ptr, unsigned len)
{
	const char *hexdig = "0123456789abcdef";
	char *ret, *p;
	unsigned i;
	ret = malloc(len*3 + 1); /* worst case size */
	if (!ret) return NULL;
	for (p=ret,i=0;i<len;i++) {
		if (isalnum(ptr[i]) || isspace(ptr[i]) ||
		    (ispunct(ptr[i]) && !strchr("\\{}", ptr[i]))) {
			*p++ = ptr[i];
		} else {
			unsigned char c = *(unsigned char *)(ptr+i);
			if (c == 0 && all_zero(ptr+i, len-i)) break;
			p[0] = '\\';
			p[1] = hexdig[c>>4];
			p[2] = hexdig[c&0xF];
			p += 3;
		}
	}

	*p = 0;

	return ret;
}

static char *decode_bytes(const char *s, unsigned *len) 
{
	char *ret, *p;
	unsigned i;
	ret = calloc(1, strlen(s)+1); /* worst case length */

	if (*s == '{') s++;

	for (p=ret,i=0;s[i];i++) {
		if (s[i] == '}') {
			break;
		} else if (s[i] == '\\') {
			unsigned v;
			if (sscanf(&s[i+1], "%02x", &v) != 1 || v > 255) {
				free(ret);
				return NULL;
			}
			*(unsigned char *)p = v;
			p++;
			i += 2;
		} else {
			*p++ = s[i];
		}
	}
	*p = 0;

	(*len) = (unsigned)(p - ret);
	
	return ret;
}

static int addgen_alloc(struct parse_string *p, int n)
{
	if (p->length + n <= p->allocated) return 0;
	p->allocated = p->length + n + 100;
	p->s = realloc(p->s, p->allocated);
	if (!p->s) {
		errno = ENOMEM;
		return -1;
	}
	return 0;
}

static int addchar(struct parse_string *p, char c)
{
	if (addgen_alloc(p, 2) != 0) {
		return -1;
	}
	p->s[p->length++] = c;
	p->s[p->length] = 0;
	return 0;
}

static int addstr(struct parse_string *p, const char *s)
{
	int len = strlen(s);
	if (addgen_alloc(p, len+1) != 0) {
		return -1;
	}
	memcpy(p->s + p->length, s, len+1);
	p->length += len;
	return 0;
}

static int addtabbed(struct parse_string *p, const char *s, unsigned indent)
{
	int len = strlen(s);
	if (addgen_alloc(p, indent+len+1) != 0) {
		return -1;
	}
	while (indent--) {
		p->s[p->length++] = '\t';
	}
	memcpy(p->s + p->length, s, len+1);
	p->length += len;
	return 0;
}

/* note! this can only be used for results up to 60 chars wide! */
static int addgen(struct parse_string *p, const char *fmt, ...)
{
	char buf[60];
	int n;
	va_list ap;
	va_start(ap, fmt);
	n = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	if (addgen_alloc(p, n + 1) != 0) {
		return -1;
	}
	if (n != 0) {
		memcpy(p->s + p->length, buf, n);
	}
	p->length += n;
	p->s[p->length] = 0;
	return 0;
}

static int gen_dump_base(struct parse_string *p, 
			  enum parse_type type, 
			  const char *ptr)
{
	switch (type) {
	case T_TIME_T:
		return addgen(p, "%u", *(time_t *)(ptr));
	case T_UNSIGNED:
		return addgen(p, "%u", *(unsigned *)(ptr));
	case T_DOUBLE:
		return addgen(p, "%lg", *(double *)(ptr));
	case T_INT:
		return addgen(p, "%d", *(int *)(ptr));
	case T_LONG:
		return addgen(p, "%ld", *(long *)(ptr));
	case T_ULONG:
		return addgen(p, "%lu", *(unsigned long *)(ptr));
	case T_CHAR:
		return addgen(p, "%u", *(unsigned char *)(ptr));
	case T_FLOAT:
		return addgen(p, "%g", *(float *)(ptr));
	case T_STRUCT:
	case T_ENUM:
		/* handled elsewhere */
		return -1;
	}
	return 0;
}

static int gen_dump_enum(struct parse_string *p, 
			 const struct enum_struct *einfo,
			 const char *ptr)
{
	unsigned v = *(unsigned *)ptr;
	int i;
	for (i=0;einfo[i].name;i++) {
		if (v == einfo[i].value) {
			addstr(p, einfo[i].name);
			return 0;
		}
	}
	/* hmm, maybe we should just fail? */
	return addgen(p, "%u", v);
}

static int gen_dump_one(struct parse_string *p, 
			 const struct parse_struct *pinfo,
			 const char *ptr,
			 unsigned indent)
{
	if (pinfo->type == T_STRUCT) {
		char *s = gen_dump(pinfo->pinfo, ptr, indent+1);
		if (!s) return -1;
		if (addstr(p, "{\n") || 
		    addstr(p,s) || 
		    addtabbed(p,"}", indent)) {
			free(s);
			return -1;
		}
		free(s);
		return 0;
	}

	if (pinfo->type == T_CHAR && pinfo->ptr_count == 1) {
		char *s = encode_bytes(ptr, strlen(ptr));
		if (addchar(p,'{') || 
		    addstr(p, s) ||
		    addchar(p,'}')) {
			free(s);
			return -1;
		}
		return 0;
	}

	if (pinfo->type == T_ENUM) {
		return gen_dump_enum(p, pinfo->einfo, ptr);
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

	/* special handling of fixed length strings */
	if (array_len != 0 && 
	    pinfo->ptr_count == 0 &&
	    pinfo->type == T_CHAR) {
		char *s = encode_bytes(ptr, array_len);
		if (!s) return -1;
		if (addtabbed(p, pinfo->name, indent) ||
		    addstr(p, " = {") ||
		    addstr(p, s) ||
		    addstr(p, "}\n")) {
			free(s);
			return -1;
		}
		return 0;
	}

	for (i=0;i<array_len;i++) {
		const char *p2 = ptr;
		unsigned size = pinfo->size;

		/* generic pointer dereference */
		if (pinfo->ptr_count) {
			p2 = *(const char **)ptr;
			size = sizeof(void *);
		}
		
		if ((count || pinfo->ptr_count) && 
		    pinfo->type != T_ENUM && 
		    all_zero(ptr, size)) {
			ptr += size;
			continue;
		}
		if (count == 0) {
			if (addtabbed(p, pinfo->name, indent) ||
			    addgen(p, " = %u:", i)) {
				return -1;
			}
		} else {
			if (addgen(p, ", %u:", i) != 0) {
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

	/* this allows for constant lengths */
	if (isdigit(*var)) {
		return atoi(var);
	}

	for (i=0;pinfo[i].name;i++) {
		if (strcmp(pinfo[i].name, var) == 0) break;
	}
	if (!pinfo[i].name) return -1;

	ptr = data + pinfo[i].offset;

	switch (pinfo[i].type) {
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
	p.allocated = 0;
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

		if (pinfo[i].array_len) {
			unsigned len = pinfo[i].array_len;
			if (pinfo[i].flags & FLAG_NULLTERM) {
				if (size == 1) {
					len = strnlen(ptr, pinfo[i].array_len) + 1;
				} else {
					for (len=0;len<pinfo[i].array_len;len++) {
						if (all_zero(ptr+len*size, size)) break;
					}
					len++;
				}
			}
			if (gen_dump_array(&p, &pinfo[i], ptr, 
					   len, indent)) {
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
				p2.dynamic_len = NULL;
				if (gen_dump_array(&p, &p2, *(char **)ptr, 
						   len, indent) != 0) {
					goto failed;
				}
			}
			continue;
		}

		if (pinfo[i].type != T_ENUM && all_zero(ptr, size)) continue;

		/* generic pointer dereference */
		if (pinfo[i].ptr_count) {
			ptr = *(const char **)ptr;
		}

		if (addtabbed(&p, pinfo[i].name, indent) ||
		    addstr(&p, " = ") ||
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
		switch (*s) {
		case '}':
			depth--;
			break;
		case '{':
			depth++;
			break;
		}
		if (depth == 0 && *s == c) {
			return s;
		}
		s++;
	}
	return s;
}

static int gen_parse_enum(const struct enum_struct *einfo, 
			  char *ptr, 
			  const char *str)
{
	unsigned v;
	int i;

	if (isdigit(*str)) {
		if (sscanf(str, "%u", &v) != 1) {
			errno = EINVAL;
			return -1;
		}
		*(unsigned *)ptr = v;
		return 0;
	}

	for (i=0;einfo[i].name;i++) {
		if (strcmp(einfo[i].name, str) == 0) {
			*(unsigned *)ptr = einfo[i].value;
			return 0;
		}
	}

	/* unknown enum value?? */
	return -1;
}

static int gen_parse_base(const struct parse_struct *pinfo, 
			  char *ptr, 
			  const char *str)
{
	if (pinfo->type == T_CHAR && pinfo->ptr_count==1) {
		unsigned len;
		char *s = decode_bytes(str, &len);
		if (!s) return -1;
		*(char **)ptr = s;
		return 0;
	}

	if (pinfo->ptr_count) {
		struct parse_struct p2 = *pinfo;
		*(void **)ptr = calloc(1, pinfo->ptr_count>1?sizeof(void *):pinfo->size);
		if (! *(void **)ptr) {
			return -1;
		}
		ptr = *(char **)ptr;
		p2.ptr_count--;
		return gen_parse_base(&p2, ptr, str);
	}

	switch (pinfo->type) {
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
	case T_ENUM:
		return gen_parse_enum(pinfo->einfo, ptr, str);

	case T_UNSIGNED:
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
	if (array_len != 0 && 
	    pinfo->ptr_count == 0 &&
	    pinfo->type == T_CHAR) {
		unsigned len = 0;
		char *s = decode_bytes(str, &len);
		if (!s) return -1;
		memset(ptr, 0, array_len);
		memcpy(ptr, s, len);
		free(s);
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
			unsigned size;
			struct parse_struct p2 = pinfo[i];
			char *ptr;
			size = pinfo[i].ptr_count>1?sizeof(void*):pinfo[i].size;
			ptr = calloc(len, size);
			if (!ptr) {
				errno = ENOMEM;
				return -1;
			}
			*((char **)(data + pinfo[i].offset)) = ptr;
			p2.ptr_count--;
			p2.dynamic_len = NULL;
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
