#include <stdio.h>
#include <unistd.h>

main()
{
	unsigned short x;
	int i;
	int count = 0;

	for (i=0;;i++) {
		if (read(0, &x, 2) != 2) break;
		if (x != i && x != 0) {
			count++;
		}
	}
	printf("count=%d i=%d\n", count, i);
	return 0;
}
