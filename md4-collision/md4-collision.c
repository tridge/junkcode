#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define uint32 uint32_t

#include "mdfour.h"


struct md4_result {
	unsigned data[16/sizeof(unsigned)];
};

#define HASH_BITS 22
#define BUCKETS 12

static struct md4_result results[1<<HASH_BITS][BUCKETS];

int main(void)
{
	int fd;
	int count=0;
	char buf[1024];
	struct md4_result zero;

	fd = open("/dev/urandom", O_RDONLY);
	if (fd == -1) {
		perror("/dev/urandom");
		return -1;
	}

	memset(zero.data, 0, 16);

	while (1) {
		struct md4_result r;
		int i, b;

		size_t size = 200 + random() % (sizeof(buf)-200);
		if (read(fd, buf, size) != size) {
			printf("failed to init random bytes\n");
			exit(1);
		}

		mdfour((unsigned char *)r.data, buf, size);

		i = r.data[0] & ((1<<HASH_BITS)-1);
		for (b=0;b<BUCKETS;b++) {
			struct md4_result *old = &results[i][b];
			if (memcmp(old->data, zero.data, 16) == 0) {
				results[i][b] = r;
				break;
			} else if (memcmp(old->data, r.data, 16) == 0) {
				printf("\nfound a collision after %d calls! (%d/%d)\n",
				       count, i, b);
				exit(1);
			}
		}
		if (b == BUCKETS) {
			printf("\nout of buckets at %d\n", i);
			exit(1);
		}

		count++;
		if (count % 1000 == 0) {
			printf("%7d\r", count);
			fflush(stdout);
		}
	}
	
}

