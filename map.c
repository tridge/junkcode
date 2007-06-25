#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

main()
{
	int fd = open("/dev/null", O_WRONLY);

	mmap(0, 0x1000, PROT_READ, MAP_PRIVATE, fd, 0);
}
