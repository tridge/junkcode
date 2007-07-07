/* 
   testing of alloc_mmap routines.

   Copyright (C) Andrew Tridgell 2007

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

static struct timeval timeval_current(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv;
}

static double timeval_elapsed(struct timeval *tv)
{
	struct timeval tv2 = timeval_current();
	return (tv2.tv_sec - tv->tv_sec) + 
	       (tv2.tv_usec - tv->tv_usec)*1.0e-6;
}


static int test_random(size_t maxsize)
{
	uint32_t *r;
	int i, numrand = 1000, n=500000;
	void **p;

	srandom(0);
	r = calloc(sizeof(uint32_t), numrand);
	for (i=0;i<numrand;i++) {
		r[i] = random() % maxsize;
	}

	p = calloc(sizeof(void*), n);

	for (i=0;i<n;i++) {
		p[i] = malloc(r[i%numrand]);
		assert(p[i] != NULL);
	}
	
	for (i=0;i<n;i++) {
		switch (i%2) {
		case 0:
			free(p[i]);
			p[i] = NULL;
			break;
		case 1:
			p[i] = realloc(p[i], r[(i*3)%numrand]);
			assert(p[i] != NULL || r[(i*3)%numrand] == 0);
			break;
		case 2:
			free(p[i]);
			p[i] = malloc(r[(i*5)%numrand]);
			assert(p[i] != NULL);
			break;
		}
	}
	for (i=0;i<n;i++) {
		if (p[i] != NULL) free(p[i]);
	}

	return 0;	
}

int main(void)
{
	int ret = 0;

	ret |= test_random(100);

	return ret;
}

