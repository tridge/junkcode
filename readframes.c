/* 
   test reading image frames from a directory
   tridge@samba.org, March 2006
*/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>


#define READ_SIZE 61440

static double total_time, min_time, max_time, total_bytes;
static int num_files;

static struct timeval tp1,tp2;

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

static void test_file(const char *fname)
{
	int fd;
	unsigned char buf[READ_SIZE];
	
	double t;
	int n;

	start_timer();
	fd = open(fname, O_RDONLY);
	if (fd == -1) {
		printf("Failed to open %s\n", fname);
		return;
	}
	while ((n=read(fd, buf, READ_SIZE)) > 0) {
		total_bytes += n;
	}
	close(fd);
	t = end_timer();

	if (t > max_time) max_time = t;
	if (min_time == 0 || t < min_time) min_time = t;
	total_time += t;
	num_files++;

	printf("%6.2fms %s\n", t*1000.0, fname);
}

int main(int argc, char* argv[])
{
	int i;	

	printf("readframes tester - tridge@samba.org\n");

	if (argc < 2) {
		printf("Usage: readframes <files>\n");
		exit(1);
	}

	for (i=1;i<argc;i++) {
		test_file(argv[i]);
	}

	printf("\nProcessed %d files totalling %.2f MBytes\n", 
	       num_files, total_bytes/(1024*1024));
	printf("Speed was %.2f files/sec\n", num_files/total_time);
	printf("Average speed was %.2fms per file\n", 1000.0*total_time/num_files);
	printf("Worst: %.2fms  Best: %.2fms\n", max_time*1000.0, min_time*1000.0);

        return 0;
}
