#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
	char *fname = argv[1];
	off_t megs = atoi(argv[2]);
	int fd;

	fd = open(fname,O_WRONLY|O_CREAT, 0600);
	lseek(fd, megs * 1024*1024, SEEK_SET);
	write(fd, fname, strlen(fname));
	close(fd);
	return 0;
}
