/* 
   Copyright (C) Andrew Tridgell 1996
   Copyright (C) Paul Mackerras 1996
   
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

int csum_length=2; /* initial value */

#define CSUM_CHUNK 64

int checksum_seed = 0;
extern int remote_version;

/*
  a simple 32 bit checksum that can be upadted from either end
  (inspired by Mark Adler's Adler-32 checksum)
  */
uint32
get_checksum1(char *buf1,int len)
{
    int i;
    uint32 s1, s2;
    schar *buf = (schar *)buf1;

    s1 = s2 = 0;
    for (i = 0; i < (len-4); i+=4) {
	s2 += 4*(s1 + buf[i]) + 3*buf[i+1] + 2*buf[i+2] + buf[i+3] + 
	  10*CHAR_OFFSET;
	s1 += (buf[i+0] + buf[i+1] + buf[i+2] + buf[i+3] + 4*CHAR_OFFSET); 
    }
    for (; i < len; i++) {
	s1 += (buf[i]+CHAR_OFFSET); s2 += s1;
    }
    return (s1 & 0xffff) + (s2 << 16);
}


int
get_checksum2(char *buf,int len,char *sum)
{
	int i;
	static char *buf1;
	static int len1;
	struct mdfour m;
	char tsum[MD4_LENGTH];

	if (len > len1) {
		if (buf1) free(buf1);
		buf1 = (char *)malloc(len+4);
		len1 = len;
		errno = ENOMEM;
		if (!buf1) return -1;
	}
	
	mdfour_begin(&m);
	
	memcpy(buf1,buf,len);
	if (checksum_seed) {
		SIVAL(buf1,len,checksum_seed);
		len += 4;
	}
	
	for(i = 0; i + CSUM_CHUNK <= len; i += CSUM_CHUNK) {
		mdfour_update(&m, (uchar *)(buf1+i), CSUM_CHUNK);
	}
	if (len - i > 0) {
		mdfour_update(&m, (uchar *)(buf1+i), (len-i));
	}
	
	mdfour_result(&m, (uchar *)tsum);

	memcpy(sum, tsum, SUM_LENGTH);

	return 0;
}
