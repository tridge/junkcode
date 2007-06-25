#include <stdio.h>

main(int argc, char *argv[])
{
	long long x;
	double d;

	x = atoi(argv[1]);

	d = (double)x;

	printf("%.0f %d\n", (double)x, (int)x);
}
