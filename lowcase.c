/* 
   Unix SMB/Netbios implementation.
   Version 3.0
   Copyright (C) Andrew Tridgell 2001
   
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
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

/* 
   given an upcase table file, create a lowcase table file
*/

typedef unsigned short smb_ucs2_t;

static smb_ucs2_t upcase[0x10000], lowcase[0x10000];

int main(void)
{
	int fd, i;
	char *fname = "upcase.dat";

	fd = open(fname, O_RDONLY, 0);
	if (fd == -1) {
		perror(fname);
		exit(1);
	}

	if (read(fd, upcase, 0x20000) != 0x20000) {
		printf("%s is too short\n", fname);
		exit(1);
	}

	for (i=0;i<0x10000;i++) {
		if (upcase[i] != i) lowcase[upcase[i]] = i;
	}

	for (i=0;i<0x10000;i++) {
		if (lowcase[i] == 0) lowcase[i] = i;
	}

	close(fd);

	fname = "lowcase.dat";

	fd = open(fname, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	if (fd == -1) {
		perror(fname);
		exit(1);
	}

	write(fd, lowcase, 0x20000);
	close(fd);
	return 0;
}
