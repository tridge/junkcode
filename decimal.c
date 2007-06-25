#include <math.h>

/*
a0c7 7694 ab42 7b80 a4f9 0381 382f 7680 
ab42 7b80 58e6 1199 0000 0002 

d0c7 7694 64cc 7d80 64cc 7d80 5308 6c80
7ea5 7380 9070 5096 0000 0002 
*/
decimal32 d;

main()
{
	d = 19123376;
	printf("0x%x\n", *(unsigned *)&d);
	return 0;
}
