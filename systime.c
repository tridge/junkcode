#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <unistd.h>

static struct timeval tp1,tp2;

static double tvdiff(struct timeval *tv1, struct timeval *tv2)
{
	return (tv2->tv_sec + (tv2->tv_usec*1.0e-6)) - 
		(tv1->tv_sec + (tv1->tv_usec*1.0e-6));
}

static void start_timer()
{
	gettimeofday(&tp1,NULL);
}

static double end_timer()
{
	gettimeofday(&tp2,NULL);
	fflush(stdout);
	return tvdiff(&tp1, &tp2);
}


static void load_file(void)
{
	start_timer();
	while (end_timer() < 5) {
		int i;
		for (i=0;i<1000;i++) {
			struct stat st;
			stat(".", &st);
		}
	}
}

static void load_cpu(void)
{
	start_timer();
	while (end_timer() < 5) {
		int i;
		for (i=0;i<1000;i++) {
			char x1[10000];
			char x2[10000];
			memcpy(x1, x2, sizeof(x2));
		}
	}
}


int main(void)
{
	struct rusage r1, r2;

	getrusage(RUSAGE_SELF, &r1);
	load_file();
	getrusage(RUSAGE_SELF, &r2);

	printf("file system %g\n", tvdiff(&r1.ru_stime, &r2.ru_stime));
	printf("file user   %g\n", tvdiff(&r1.ru_utime, &r2.ru_utime));

	getrusage(RUSAGE_SELF, &r1);
	load_cpu();
	getrusage(RUSAGE_SELF, &r2);
	printf("cpu  system %g\n", tvdiff(&r1.ru_stime, &r2.ru_stime));
	printf("cpu  user   %g\n", tvdiff(&r1.ru_utime, &r2.ru_utime));

	printf("\n");
	return 0;
}
