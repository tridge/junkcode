#include <stdio.h>
#include <stdlib.h>

#define TALLOC_ALIGN 32

main()
{
	size_t size;
	for (size=0;size<100;size++) {
		size_t size2 = (size + (TALLOC_ALIGN-1)) & ~(TALLOC_ALIGN-1);
		printf("size=%d size2=%d\n", (int)size, (int)size2);
	}
}
