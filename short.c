#include <stdio.h>

int main(void)
{
	unsigned short x1, x2, x3;

	x1 = 0xF000;
	x2 = 0xF000;
	x3 = 0xFF00;

	if (x1 + x2 > x3) {
		printf("ok\n");
	} else {
		printf("oops\n");
	}
	return 0;
}
