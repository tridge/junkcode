#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	int *buf;
	int size;
	int i, count;

	if (argc < 2) {
		printf("memtest <size>\n");
		exit(1);
	}

	size = atoi(argv[1]);

	buf = (int *)malloc(size);
	
	size /= sizeof(*buf);

	count = 0;

	for (i=0;i<size;i++)
		buf[i] = i;

	while (count < (1<<30)) {
		i = random() % size;

		if (buf[i] != i)
			printf("\nbuf[%d]=%d\n", i, buf[i]);

		count++;
		if (count % 100000 == 0) 
			printf("%d\r", count);
		fflush(stdout);

		buf[i] = i;
	}

	return 0;
}
