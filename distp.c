#include <stdio.h>
#include <stdlib.h>
#include <math.h>


struct rec {
	int d;
	int x;
};


static int comp(struct rec *r1, struct rec *r2)
{
	return r1->x - r2->x;
}

void shuffle(struct rec *d, int n)
{
	int i;

	for (i=0;i<n;i++)
		d[i].x = random();

	qsort(d, n, sizeof(*d), comp);
}

int count(struct rec *d, int n, int x)
{
	int i, ret=0;

	for (i=0;i<n;i++)
		if (d[i].d == x) ret++;
	return ret;
}


static double avg(struct rec *d, int n, int loops)
{
	int sum=0;
	int i;
	
	for (i=0;i<loops;i++) {
		shuffle(d, 2*n);
		sum += abs(n/2 - count(d, n, 1));
	}
	return sum/(1.0*i);
}

static double Pn(int n)
{
	return 0.20 * sqrt(1.*n);
}

static int log2(int n)
{
	int i;
	for (i=1;(1<<i)<n;i++) ;
	return i;
}

int main(int argc, char *argv[])
{
	struct rec *d;
	int i;
	int N = atoi(argv[1]);
	int P = atoi(argv[2]);
	double r=0;
	double k=0.4;

	r = log2(P/4)*2*(P-3)*k*sqrt(N/P);

	printf("P=%d N=%d Pn=%g\n", P, N/P, r);
}

