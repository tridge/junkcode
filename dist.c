#include <stdio.h>
#include <stdlib.h>
#include <math.h>


static int loops;

struct rec {
	int d;
	int x;
};


static int comp(struct rec *r1, struct rec *r2)
{
	return r1->x - r2->x;
}

static int comp2(struct rec *r1, struct rec *r2)
{
	return r1->d - r2->d;
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

static double countr(struct rec *d, int n, int loops, int r)
{
	int sum=0;
	int i;
	
	for (i=0;i<loops;i++) {
		shuffle(d, 2*n);
		if (count(d, n, 1) == r) sum++;
	}
	return sum/(1.*loops);
}


static double sqr(double x)
{
	return x*x;
}

static double fact(int n)
{
	int i;
	double ret=1;
	for (i=2;i<=n;i++)
		ret *= i;
	return ret;
}

static double binomial(int n, int k)
{
	return fact(n) / (fact(k) * fact(n-k));
}

static double P0(int n)
{
	return sqr(fact(n/2)) / fact(n);
}

static double P1(int n)
{
	return P0(n) * (n/2) * (n/2);
}

static double P2(int n)
{
	return P0(n) * (n/2) * (n/2 - 1)/2 * binomial(n/2, 2);
}

static double Pn(int n, int r)
{
	return sqr(binomial(n/2, r)) * sqr(fact(n/2)) / fact(n);
}

static double En(int n)
{
	return (n/2) * (sqr(fact(n/2))/fact(n)) * binomial(n-1, n/2-1);
}


/*
  > p;

                                           2     2
                             binomial(n, i)  (n!)
                             ---------------------
                                     (2 n)!


> 4*sum(p*(n/2 - i),i=0..n/2);                      

                        2                       2            2
                    (n!)  binomial(n, 1 + 1/2 n)  (1 + 1/2 n)
                  2 ------------------------------------------
                                     (2 n)! n


This is the expected number of elements in the wrong cell after the
hypercube phase of a 4 way internal sort.

 */


/* take n random elements. Sample k of those elements. How far is the median
   of the n elements from the median of the k elements? Answer in terms
   of the k elements (so the answer is at most k/2) */
static int median_deviation(int n, int k)
{
	struct rec *d;
	int i, j;
	double s;
	double sum;

	srandom(getpid() ^ time(NULL));

	d = (struct rec *)malloc(sizeof(d[0])*n);

	for (i=0;i<n;i++) {
		d[i].d = i;
	}

	sum = 0;
	for (j=0;j<loops;j++) {
		shuffle(d, n);
	
		qsort(d, k, sizeof(*d), comp2);
		
		sum += abs(n/2 - d[k/2].d);
	}
	sum /= j;

	free(d);

	return sum;
}


int main(int argc, char *argv[])
{
	struct rec *d;
	int i;
	int N = atoi(argv[1]);
	int k = atoi(argv[2]);

	loops = atoi(argv[3]);

	printf("median_deviation(N, k) = %g\n", median_deviation(N, k));
}
