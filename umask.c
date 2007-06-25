#include <stdio.h>
main()
{
	printf("umask=0%o\n", umask(0));
	sleep(1000);
}
