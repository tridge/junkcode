#include <stdlib.h>
#include <stdio.h>

int main(void)
{
	int i;
	time_t t, t2;
	struct timeval tv, tv2;

	t = time(NULL);

	i=0;

	while (1) {
		t2 = time(NULL);
		i++;
		if (t2 - t > 10) break;
	}
	
	printf("time(): %g ops/sec\n", (1.0*i) / (t2-t));


	gettimeofday(&tv, NULL);
	i=0;

	while (1) {
		gettimeofday(&tv2, NULL);
		i++;
		if (tv2.tv_sec - tv.tv_sec > 10) break;
	}
	
	printf("gettimeofday(): %g ops/sec\n", (1.0*i) / (tv2.tv_sec - tv.tv_sec));
	return 0;
}
