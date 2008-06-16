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
#include <malloc.h>

static int test_random(size_t maxsize, int n)
{
	uint32_t *r;
	int i, numrand = 1000, loops=7;
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
	
	while (loops--) {
		for (i=0;i<n;i++) {
			switch (r[(loops*i)%numrand]%5) {
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
			case 3:
				cfree(p[i]);
				p[i] = calloc(i%4, r[(i*5)%numrand]);
				assert(p[i] != NULL);
				break;
			case 4:
				free(p[i]);
				if (random() % 100 == 0) {
					int boundary = (1<<r[(i*7)%numrand]%3)*4096;
					int ret = posix_memalign(&p[i], 
								 boundary,
								 r[(i*7)%numrand]);
					assert(ret==0);
				} else {
					p[i] = memalign((1<<(i%5)), r[(i*7)%numrand]);
				}
				assert(p[i] != NULL);
				break;
			}
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

	setlinebuf(stdout);

	ret |= test_random(100,   500000);
	ret |= test_random(2000,  10000);
	ret |= test_random(8000,  1000);

	return ret;
}

