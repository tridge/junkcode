#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

static struct timeval tp1,tp2;

static void start_timer()
{
	gettimeofday(&tp1,NULL);
}

static double end_timer()
{
	gettimeofday(&tp2,NULL);
	return (tp2.tv_sec + (tp2.tv_usec*1.0e-6)) - 
		(tp1.tv_sec + (tp1.tv_usec*1.0e-6));
}


static void create_loop(const char *fname)
{
	unsigned count = 0;
	int fd;
	start_timer();

	while (1) {
		count++;

		unlink(fname);
		fd = open(fname, O_RDWR|O_CREAT|O_TRUNC|O_EXCL, 0644);
		if (fd == -1) {
			perror(fname);
			exit(1);
		}
		close(fd);

		if (end_timer() > 1.0) {
			printf("%.0f ops/sec\r", count/end_timer());
			fflush(stdout);
			start_timer();
			count=0;
		}
	}
}

static void usage(void)
{
	printf("usage: create_loop <file>\n");
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		usage();
		exit(1);
	}
	create_loop(argv[1]);
	return 0;
}
