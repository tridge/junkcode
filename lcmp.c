#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	int fd1, fd2;
	char *buf1, *buf2;
	int bufsize = 1024*1024;
	off_t offset;

	if (argc < 3) {
		printf("usage: lcmp <file1> <file2>\n");
		exit(1);
	}

	fd1 = open(argv[1], O_RDONLY);
	fd2 = open(argv[2], O_RDONLY);

	buf1 = malloc(bufsize);
	buf2 = malloc(bufsize);

	offset = 0;

	while (1) {
		int n1, n2, n, i;

		printf("%.0f\r", (double)offset);
		fflush(stdout);

		n1 = read(fd1, buf1, bufsize);
		n2 = read(fd2, buf2, bufsize);

		n = n1;
		if (n2 < n1) n = n2;

		if (memcmp(buf1, buf2, n)) {
			for (i=0;i<n;i++)
				if (buf1[i] != buf2[i]) {
					printf("%s and %s differ at offset %.0f\n",
					       argv[1], argv[2], (double)(offset+i));
					exit(1);
				}
		}

		if (n1 < n2) {
			printf("EOF on %s\n", argv[1]);
			exit(1);
		}
		if (n2 < n1) {
			printf("EOF on %s\n", argv[2]);
			exit(1);
		}

		offset += n;

		if (n == 0) break;
	}

	free(buf1);
	free(buf2);
	close(fd1);
	close(fd2);
	return 0;
}
