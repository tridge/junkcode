#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/resource.h>

double walltime(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec + 1.0e-6*tv.tv_usec;
}

main(int argc, char *argv[])
{
	int fd;
	int i;
	char *buf;
	int bufsize, sync, loops;
	double t;

	if (argc < 3) {
		printf("usage: %s <bufsize> <sync> <loops>\n", argv[0]);
		exit(1);
	}

	bufsize = atoi(argv[1]);
	sync = atoi(argv[2]);
	loops = atoi(argv[3]);

	buf = (char *)malloc(bufsize);
	if (!buf) exit(1);
	memset(buf, 1, bufsize);

	fd = open("sync.dat", O_CREAT|O_TRUNC|O_WRONLY | (sync?O_SYNC:0),0600);
	t = walltime();

	for (i=0;i<loops;i++) {
		write(fd, buf, bufsize);
	}

	t = walltime() - t;

	printf("%g MB/sec\n", (1.0e-6*i*bufsize)/t);

	unlink("sync.dat");
}
