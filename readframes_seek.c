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
#include <ctype.h>


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

static void test_file(const char *fname, int seeksize)
{
	int fd;
	unsigned char buf[64000];
	double t;
	double lastt = total_time;

	fd = open(fname, O_RDONLY);
	if (fd == -1) {
		printf("Failed to open %s\n", fname);
		return;
	}

	while (1) {
		start_timer();
		/* for the app, reading a frame involves 5 reads and a
		   seek */
		read(fd, buf, 61440);
		read(fd, buf, 61440);
		read(fd, buf, 61440);
		read(fd, buf, 61440);
		if (read(fd, buf, 42240) != 42240) break;
		lseek(fd, seeksize, SEEK_CUR);
		total_bytes += 42240 + 4*61440;
		t = end_timer();
		total_time += t;
		if (t > max_time) {
			max_time = t;
		}
		if (min_time == 0 || t < min_time) {
			min_time = t;
		}
		if (total_time > lastt + 1.0) {
			lastt = total_time;
			printf("Worst: %.2fms  Best: %.2fms  throughput=%.2f MByte/sec\n", 
			       max_time*1000.0, min_time*1000.0, 
			       1.0e-6*total_bytes/total_time);
		}
	}

	num_files++;
	close(fd);
}

int main(int argc, char* argv[])
{
	int i;	
	int seeksize;

	printf("readframes_seek tester - tridge@samba.org\n");

	setlinebuf(stdout);

	if (argc < 3) {
		printf("Usage: readframes_seek <seeksize> <files>\n");
		exit(1);
	}

	seeksize = strtoul(argv[1], NULL, 0);

	for (i=2;i<argc;i++) {
		test_file(argv[i], seeksize);
	}

	printf("\nProcessed %d files totalling %.2f MBytes with seeksize=%d\n", 
	       num_files, total_bytes/(1024*1024), seeksize);
	printf("Worst: %.2fms  Best: %.2fms\n", max_time*1000.0, min_time*1000.0);
	printf("Throughput %.2f MByte/sec\n", 1.0e-6*total_bytes/total_time);

        return 0;
}
