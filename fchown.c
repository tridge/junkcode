#include <sys/types.h>
#include <sys/stat.h>
#include <asm/fcntl.h>

main()
{
	int fd = open(".", O_DIRECTORY|O_RDONLY);
	fchown(fd, 2015, 2015);
}
