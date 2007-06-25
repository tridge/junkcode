#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	int size = 250;
	char *p;

	if (argc > 1) {
		size = atoi(argv[1]);
	}
	
	size *= 1024 * 1024;

	p = malloc(size);
	if (!p) {
		fprintf(stderr,"Malloc failed!\n");
		exit(1);
	}

	memset(p, 0, size);
	exit(0);
}
