#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

main(int argc, char *argv[])
{
	int fd, n, total=0;
	char buffer[64*1024];

	fd = open(argv[1], O_RDONLY);

	if (fd == -1) {
		perror(argv[1]);
		exit(1);
	}

	/* lseek(fd, 300*1024*1024, SEEK_SET); */

	while (1) {
		if ((n = read(fd, buffer, sizeof(buffer))) != sizeof(buffer)) {
			printf("\neof at %d\n", total);
			break;
		}
		total += n;
		printf("read %9d bytes\r", total);
		fflush(stdout);
	}
	printf("\n");
	return 0;
}
