#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
	unsigned char *buf;
	int size;
	int i, count;

	if (argc < 2) {
		printf("memtest <size>\n");
		exit(1);
	}

	size = atoi(argv[1]);

	buf = (unsigned char *)malloc(size);
	
	count = 0;

	while (1) {
		unsigned char v = count % 256;
		memset(buf, v, size);
		for (i=0;i<size;i++) {
			if (buf[i] != v) {
				printf("\nbuf[%d]=0x%x v=0x%x\n", 
				       i, buf[i], v);
			}
		}
		count++;
		printf("%d\r", count);
		fflush(stdout);
	}

	return 0;
}
