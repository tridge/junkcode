#include <stdio.h>

typedef unsigned:1 BOOL;

int main(void)
{
	BOOL b;

	b = 3;
	
	printf("b=%u\n", (unsigned)b);

	return 0;
}
