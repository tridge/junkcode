#include <stdio.h>
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

static void attack_it(const char *target, int len)
{
	int i;
	double totals[256];
	double this_run[256];
	char teststr[len+1];
	int c, r, runs, min_c;
	double min;

	runs = 1000000;


	for (i=0;i<len;i++) {
		memset(totals, 0, sizeof(totals));

		for (r=0;r<runs;r++) {
			for (c=0;c<256;c++) {
				start_timer();
				teststr[i] = c;
				memcmp(teststr, target, i+1);
				this_run[c] = end_timer();
			}
			for (c=0;c<256;c++) {
//				printf("%3d %lf\n", c, 1000*1000*this_run[c]);
				totals[c] += this_run[c];
			}
		}

		min_c = 0;
		min = totals[0];
		for (c=1;c<256;c++) {
			if (totals[c] < min) {
				min = totals[c];
				min_c = c;
			}
		}
		printf("min_c=%d\n", min_c);
	}
}

int main(int argc, char *argv[])
{
	attack_it(argv[1], strlen(argv[1]));
	return 0;
}
