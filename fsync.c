#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

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

int main(int argc, const char *argv[])
{
	int fd;
	char *p;
	size_t size = 4096;
	const char *fname = "fsync.dat";

	unlink(fname);
	fd = open(fname, O_RDWR|O_CREAT|O_EXCL, 0600);
	if (fd == -1) {
		perror(argv[1]);
		return -1;
	}

	while (size < 100*1024*1024) {
		int count = 0;
		ftruncate(fd, size);

		p = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
		if (p == (void *)-1) {
			perror("mmap");
			return -1;
		}

		memset(p, 1, size);
		msync(p, size, MS_SYNC);
		fsync(fd);

		start_timer();
		while (end_timer() < 2) {
#if 0
			pwrite(fd, &count, 1, size-1);
			fsync(fd);
#else
			memset(p+size-100, count, 100);
//			msync(p, size, MS_SYNC);
			msync(p+size-4096, 4096, MS_SYNC);
#endif
			count++;
		}

		printf("%7d  %.2f us  (count=%d)\n", 
		       size, 1.0e6 * end_timer() / count, count);

		munmap(p, size);
		size *= 2;
	}

	close(fd);
	unlink(fname);

	return 0;
}
