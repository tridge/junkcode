/* 
   Copyright (C) by Andrew Tridgell 2001
   
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

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdarg.h>
#include <dirent.h>
#include <sys/stat.h>

typedef unsigned uint32;

#include "mdfour.h"
#include "tdb.h"


#ifdef __GNUC__
#define PRINTF_ATTRIBUTE(a1, a2) __attribute__ ((format (__printf__, a1, a2)))
#else
#define PRINTF_ATTRIBUTE(a1, a2)
#endif

int vasprintf(char **ptr, const char *format, va_list ap);
int asprintf(char **,const char *, ...) PRINTF_ATTRIBUTE(2,3);

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
