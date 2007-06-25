#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <fcntl.h>

main()
{
	int fd = open("/dev/zero", O_RDWR);
	char *map;

	map = mmap(0, 0x4000, PROT_READ | PROT_WRITE, MAP_LOCKED| MAP_PRIVATE | MAP_FILE,
		   fd, 0);
}
