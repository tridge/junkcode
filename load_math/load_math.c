#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>

double global_v;

static void alarm_handler(int sig)
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	double v = tan(sin(cos(tv.tv_sec)));
	v *= 1.3;
	global_v = v;
}

void _init(void)
{
	struct timeval tv;
	struct itimerval val;
	tv.tv_sec = 0;
	tv.tv_usec = 1;
	val.it_interval = tv;
	val.it_value = tv;

	setitimer(ITIMER_REAL, &val, NULL);
	signal(SIGALRM, alarm_handler);
}
