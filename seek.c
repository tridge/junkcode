#include <stdio.h>
#include <stdlib.h>

main(int argc, char *argv[])
{
	int fd;

	f = fopen("foo","r");

	printf("seek gives %d\n", fseek(f, 0, SEEK_END));
}
