#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	int i;
	time_t t, t2;
	int count = atoi(argv[1]);
	int size = atoi(argv[2]);

	t = time(NULL);

	for (i=0;i<count; i++) {
		char *p;
		p = malloc(size);
		if (!p) {
			printf("malloc failed!\n");
			exit(1);
		}
		free(p);
	}

	t2 = time(NULL);
	
	printf("%g ops/sec\n", (2.0*i) / (t2-t));
	return 0;
}
