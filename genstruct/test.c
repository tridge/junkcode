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
  test code for genstruct
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stddef.h>
#include <stdarg.h>
#include "genparser.h"

#include "test.h"
#include "parse_info.h"

static void fill_test2_p(struct test2 **t, int count);

static void fill_test2(struct test2 *t2, int count)
{
	t2->x1 = 7 - count;
	t2->foo = strdup("hello { there");

	t2->dlen = random() % 30;
	if (t2->dlen) {
		int i;
		t2->dfoo = malloc(t2->dlen);
		for (i=0;i<t2->dlen;i++) {
			t2->dfoo[i] = random();
		}
	}
	
	if (count) fill_test2_p(&t2->next, count-1);
}

static void fill_test2_p(struct test2 **t, int count)
{
	(*t) = calloc(1, sizeof(struct test2));

	fill_test2(*t, count);
}

static void fill_test1(struct test1 *t)
{
	int i;

	memset(t, 0, sizeof(*t));

	strcpy(t->foo, "hello foo");
	t->foo2[1] = strdup("foo2 } you");
	t->foo2[2] = strdup("foo2 you 2");

	t->xlen = random() % 10;
	if (t->xlen) {
		t->iarray = malloc(t->xlen * sizeof(t->iarray[0]));
	}
	for (i=0;i<t->xlen;i++) {
		t->iarray[i] = random() % 10;
	}

	t->slen = random() % 6;
	if (t->slen) {
		t->strings = malloc(t->slen * sizeof(char *));
	}
	for (i=0;i<t->slen;i++) {
		asprintf(&t->strings[i], "test string %u", (unsigned)(random() % 100));
	}

	for (i=2;i<4;i++) {
		asprintf(&t->s2[i], "t2 string %u", (unsigned)(random() % 100));
	}

	t->d1 = 3.5;
	t->d3 = -7;

	fill_test2_p(&t->test2, 3);

	t->alen = random() % 4;
	if (t->alen) {
		t->test2_array = calloc(t->alen, sizeof(struct test2));
	}
	for (i=0;i<t->alen;i++) {
		fill_test2(&t->test2_array[i], 2);
	}

	for (i=0;i<2;i++) {
		fill_test2_p(&t->test2_fixed[i], 1);
	}

	t->plen = random() % 4;
	if (t->plen) {
		t->test2_parray = calloc(t->plen, sizeof(struct test2 *));
	}
	for (i=0;i<t->plen;i++) {
		fill_test2_p(&t->test2_parray[i], 2+i);
	}
}

/*
  save a lump of data into a file. 
  return 0 on success
*/
int file_save(const char *fname, void *data, size_t length)
{
	int fd;
	unlink(fname);
	fd = open(fname, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (fd == -1) {
		return -1;
	}
	if (write(fd, data, length) != length) {
		close(fd);
		unlink(fname);
		return -1;
	}
	close(fd);
	return 0;
}

int main(void)
{
	char *s, *s2;
	struct test1 t, t2;

	srandom(time(NULL));
	
	fill_test1(&t);

	s = gen_dump(pinfo_test1, (char *)&t, 0);
	printf("%s\n", s);

	memset(&t2, 0, sizeof(t2));

	if (gen_parse(pinfo_test1, (char *)&t2, s) != 0) {
		printf("Parse failed!\n");
		exit(1);
	}

	s2 = gen_dump(pinfo_test1, (char *)&t2, 0);
	printf("%s\n", s2);

	if (strcmp(s, s2) != 0) {
		printf("MISMATCH!\n");
		file_save("s1", s, strlen(s));
		file_save("s2", s2, strlen(s2));
		exit(1);
	} else {
		printf("Compares OK!\n");
	}

	return 0;
}
