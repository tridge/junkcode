#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/file.h>

int main(void)
{
	int fd, fd2;
	char buf[1024]="";
	int i;
	char *p;

	memset(buf,'Z', sizeof(buf));

	fd = open("mmap.tst", O_WRONLY|O_CREAT|O_TRUNC, 0600);

	for (i=0;i<32;i++) {
		write(fd, buf, sizeof(buf));
	}

	close(fd);

	fd = open("mmap.tst", O_RDONLY);

	p = mmap(NULL, i*sizeof(buf), PROT_READ, MAP_SHARED, fd, 0);

	truncate("mmap.tst", 0);

	fd2 = open("mmap.tst.2", O_WRONLY|O_TRUNC|O_CREAT, 0600);

	printf("accessing ...\n");

	write(fd2, p, i*sizeof(buf));

	close(fd2);

	return 0;
}
