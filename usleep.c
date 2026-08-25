#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>

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

int main(int argc, char *argv[])
{
	unsigned long t;

	t = atoi(argv[1]);

	while (1) {
		start_timer();
#if 0
		usleep(t);
#else
		struct timespec ts;
		ts.tv_nsec = t*1000;
		ts.tv_sec = 0;
		nanosleep(&ts, NULL);
#endif
		printf("%f milliseconds\n", end_timer() * 1000);
	}
	return 0;
}
