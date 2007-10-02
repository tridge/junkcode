#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

static void pread_file(const char *fname)
{
	int fd;
	char buf[61440];
	off_t ofs = 0;

	fd = open(fname, O_RDONLY);
	if (fd == -1) {
		perror(fname);
		return;
	}

	while (1) {
		ssize_t ret = pread(fd, buf, sizeof(buf), ofs);
		if (ret <= 0) {
			printf("read error at ofs %lld - gave %d (%s)\n", 
			       (long long)ofs, (int)ret, strerror(errno));
			break;
		}
		ofs += ret;		
	}
	close(fd);
}

int main(int argc, const char *argv[])
{
	int i;
	for (i=1;i<argc;i++) {
		pread_file(argv[i]);
	}
	return 0;
}
