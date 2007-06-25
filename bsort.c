/* 
   Copyright (C) Andrew Tridgell 1998
   
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

/* a dumb implementation of a block sorting compressor, primarily
   written to make sure I understand the algorithm */

#include "rzip.h"

struct ch {
	uchar *p; /* pointer into original buffer */
};

static uchar *block_end;
static uchar *block_start;

static int ch_cmp(struct ch *d1, struct ch *d2)
{
	int len1, len2, len;
	int ret;
	uchar *p1=d1->p, *p2=d2->p;

	if (p1[0] != p2[0]) return p1[0] - p2[0];

	len1 = (int)(block_end - p1);
	len2 = (int)(block_end - p2);
	len = MIN(len1, len2);

	ret = memcmp(d1->p, d2->p, len);
	if (ret != 0) return ret;

	p1 += len;
	p2 += len;

	if (p1 == block_end) p1 = block_start;
	if (p2 == block_end) p2 = block_start;

	len = (block_end-block_start)-len;

	while (len--) {
		if (p1[0] != p2[0]) return p1[0] - p2[0];
		p1++; p2++;
		if (p1 == block_end) p1 = block_start;
		if (p2 == block_end) p2 = block_start;
	}

	return 0;
}

/* sort a block of data */
void block_sort(uchar *in, uchar *out, int len)
{
	struct ch *d;
	int i, Index;

	block_start = in;
	block_end = in+len;

	d = (struct ch *)malloc(len*sizeof(d[0]));
	if (!d) {
		fprintf(stderr,"not enough memory in block_sort - exiting\n");
		exit(1);
	}

	/* fill the data array */
	for (i=0;i<len-1;i++) {
		d[i].p = &in[i+1]; 
	}
	d[i].p = &in[0];

	/* sort it */
	qsort(d, len, sizeof(d[0]), ch_cmp);

	/* pull out the sorted data */
	for (i=0;i<len;i++) {
		if (d[i].p == in) {
			Index = i;
			out[i+4] = in[len-1];
		} else {
			out[i+4] = d[i].p[-1];
		}
	}	

	out[0] = Index&0xFF;
	out[1] = (Index>>8)&0xFF;
	out[2] = (Index>>16)&0xFF;
	out[3] = (Index>>24)&0xFF;

	/* cleanup */
	free(d);
}


static int uch_cmp(int *d1, int *d2)
{
	return block_start[*d1] - block_start[*d2];
}

/* unsort a block of data */
void block_unsort(uchar *in, uchar *out, int len)
{
	int Index, i, j;
	int *links;

	Index = in[0] | (in[1]<<8) | (in[2]<<16) | (in[3]<<24);

	len -= 4;
	in += 4;

	block_start = in;
	
	/* build the indexes */
	links = (int *)malloc(len*sizeof(links[0]));
	for (i=0;i<len;i++) {
		links[i] = i;
	}

	/* sort the indexes by transmitted char to reveal the links */
	qsort(links, len, sizeof(links[0]), uch_cmp);

	/* follow our tail to decode the data */
	j = links[Index];
	for (i=0;i<len;i++) {
		out[i] = in[j];
		j = links[j];
	}
	
	/* cleanup */
	free(links);
}


 int main(int argc, char *argv[])
{
	char *f1 = argv[1];
	char *f2 = argv[2];
	int fd1, fd2;
	uchar *buf1, *buf2;
	struct stat st;

	fd1 = open(f1, O_RDONLY);
	fd2 = open(f2, O_WRONLY|O_CREAT|O_TRUNC, 0666);
	
	fstat(fd1, &st);

	buf1 = (uchar *)malloc(st.st_size);
	buf2 = (uchar *)malloc(st.st_size+4);

	read(fd1, buf1, st.st_size);

	if (!strstr(argv[0],"bunsort")) {
		block_sort(buf1, buf2, st.st_size);

		write(fd2,buf2,st.st_size+4);
	} else {
		block_unsort(buf1, buf2, st.st_size);

		write(fd2,buf2,st.st_size-4);
	}

	close(fd1);
	close(fd2);
	return 0;
}
