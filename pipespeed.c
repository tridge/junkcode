#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>

#define BUFSIZE 0x8000


struct timeval tp1,tp2;

static void start_timer()
{
	gettimeofday(&tp1,NULL);
}

static double end_timer()
{
	gettimeofday(&tp2,NULL);
	return((tp2.tv_sec - tp1.tv_sec) + 
	       (tp2.tv_usec - tp1.tv_usec)*1.0e-6);
}

int main(int argc, char *argv[])
{
	int fdpair[2];
	int fd, size, i=0;
	char buf[BUFSIZE];

	if (argc < 2) {
		printf("Usage: pipespeed megabytes\n");
		exit(1);
	}

	memset(buf,'Z',BUFSIZE);

	size = atoi(argv[1]) * 0x100000;

	fd = open("/dev/null", O_WRONLY);
	if (fd == -1) {
		perror("open");
		exit(1);
	}

	if (pipe(fdpair) != 0) {
		perror("pipe");
		exit(1);
	}

	if (fork() == 0) {
		close(fdpair[1]);
		while (i<size) {
			int n = read(fdpair[0], buf, BUFSIZE);
			if (n <= 0) exit(1);
			write(fd, buf, n);
			i += n;
		}
		exit(0);
	}

	close(fdpair[0]);

	start_timer();

	while (i<size) {
		int n = write(fdpair[1], buf, BUFSIZE);
		if (n <= 0) {
			printf("pipe write error\n");
			exit(1);
		}
		i += n;
	}

	printf("%g MB/sec\n", (i/(0x100000))/end_timer());

	return 0;
}
