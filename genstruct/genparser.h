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

/* these macros are needed for genstruct auto-parsers */
#define GENSTRUCT
#define _LEN(x)

/*
  automatic marshalling/unmarshalling system for C structures
*/

enum parse_type {T_INT, T_UNSIGNED, T_CHAR,
		 T_FLOAT, T_DOUBLE, T_ENUM, T_STRUCT,
		 T_TIME_T, T_LONG, T_ULONG};

struct parse_struct {
	const char *name;
	enum parse_type type;
	unsigned ptr_count;
	unsigned size;
	unsigned offset;
	unsigned array_len;
	struct parse_struct *pinfo;
	const char *dynamic_len;
};

char *gen_dump(const struct parse_struct *pinfo, 
	       const char *data, 
	       unsigned indent);
int gen_parse(const struct parse_struct *pinfo, char *data, const char *str0);

