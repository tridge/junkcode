#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{
	int fd = open("/dev/mem", O_RDONLY);
	unsigned base, length;
	char *p;
	
	sscanf(argv[1],"%x", &base);
	sscanf(argv[2],"%x", &length);

	fprintf(stderr,"Dumping 0x%x at 0x%x\n", length, base);

	p = mmap(0, length, PROT_READ, MAP_PRIVATE, fd, base);
	if (p != (char *)-1) {
		fwrite(p, 1, length, stdout);
	}
	return 0;
}
