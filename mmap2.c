#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/file.h>

int main(void)
{
	int fd;
	int i;
	char *p;

	fd = open("mmap.tst", O_RDWR|O_CREAT|O_TRUNC, 0600);

	ftruncate(fd, 8192);

	p = mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

	while (1) {
		sleep(4);
		p[1024]++;
		pwrite(fd, p, 1, 0);
	}

	return 0;
}
