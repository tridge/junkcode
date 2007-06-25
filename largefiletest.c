#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/*
  try to determine if the filesystem supports large files
*/
static int large_file_support(const char *path)
{
	int fd;
	ssize_t ret;
	char c;

	fd = open(path, O_RDWR|O_CREAT, 0600);
	unlink(path);
	if (fd == -1) {
		/* have to assume large files are OK */
		return 1;
	}
	ret = pread(fd, &c, 1, ((unsigned long long)1)<<32);
	close(fd);
	return ret == 0;
}

int main(int argc, const char *argv[])
{
	int s = large_file_support(argv[1]);
	printf("sizeof(off_t)=%d s=%d\n", sizeof(off_t), s);
	return 0;
}
