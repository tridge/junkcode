#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(void)
{
	int i;
	for (i=0;i<32;i++) {
		printf("%3d  %s\n", i, strerror(i));
	}
	return 0;
}
