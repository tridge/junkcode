#include <stdio.h>

typedef struct {
	unsigned val:32;
} uint32;

int main(void)
{
	uint32 x;
	x = 5;
	printf("sizeof(x)=%d\n", sizeof(x));
	return 0;
}
