#include <unistd.h>
#include <sys/time.h>
#include <time.h>

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
		usleep(t);
		printf("%f milliseconds\n", end_timer() * 1000);
	}
	return 0;
}
