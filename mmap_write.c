#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/file.h>

int main(int argc, const char *argv[])
{
	int fd;
	char *p;

	fd = open(argv[1], O_RDWR, 0600);
	if (fd == -1) {
		perror(argv[1]);
		return -1;
	}

	p = mmap(NULL, 4, PROT_WRITE, MAP_SHARED, fd, 0);
	if (p == (void *)-1) {
		perror("mmap");
		return -1;
	}

	p[0]++;

	munmap(p, 4);
	close(fd);

	return 0;
}
