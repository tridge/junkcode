#include <stdio.h>

int main(void)
{
	printf("%*d\n", 3, 12);
	printf("%2$*1$.*3$f\n", 3, 12.12345, 2);
	return 0;
}
