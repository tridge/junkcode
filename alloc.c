#include <stdio.h>
#include <malloc.h>

static int sum;

char *xmalloc(int size)
{
	char *ret;
	sum += size;
	ret = malloc(size);
	if (!ret) {
		fprintf(stderr,"out of memory\n");
		exit(1);
	}
	memset(ret, 1, size);
	return ret;
}

main(int argc, char *argv[])
{
	int n = atoi(argv[1]);
	int i;
	
	for (i=0;i<n;i++) {
		xmalloc(56);
		xmalloc(9);
	}
	
	printf("allocated %d bytes\n", sum);
	sleep(20);
}
