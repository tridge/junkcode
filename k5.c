#include <stdio.h>

main(int argc, char *argv[])
{
	int x1=0;
	sscanf(argv[1], "%x", &x1);
	printf("%d\n", x1);
}
