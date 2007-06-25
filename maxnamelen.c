#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

int main(void)
{
	int i, fd;

	for (i=1;i<1000;i++) {
		char *fname;
		fname = malloc(i+1);
		memset(fname, 'X', i);
		fname[i] = 0;
		fd = open(fname, O_CREAT|O_EXCL|O_RDWR, 0644);
		if (fd == -1) {
			printf("failed at %d - %s\n", i, strerror(errno));
			break;
		}
		close(fd);
		unlink(fname);
	}

	return -1;
}
