#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>

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


char buf[1024*1024];

int main(int argc, char *argv[])
{
	char *fname = argv[1];
	int fd;
	int count=0;

	fd = open(fname,O_RDONLY);

	while (1) {
		int ret;

		if (count == 0) {
			start_timer();
		}
		
		ret = read(fd, buf, sizeof(buf));

		lseek(fd, 0, SEEK_SET);

		if (count++ == 10) {
			printf("%g MB/sec\n", count*ret/(1.0e6*end_timer()));
			count=0;
		}
	}

	return 0;
}
