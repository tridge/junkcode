#include <stdlib.h>
#include <fcntl.h>

int main()
{
	int i;

	for (i=0; ; i++)
		if (open("/dev/null", O_RDONLY) == -1) break;

	printf("opened %d files\n", i);
	sleep(100);
}
