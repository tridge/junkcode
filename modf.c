#include <stdio.h>
#include <math.h>

/* a replacement for modf that doesn't need the math library. Should
   be portable, but slow */
static double my_modf(double x0, double *iptr)
{
	int i;
	long l;
	double x = x0;
	double f = 1.0;

	for (i=0;i<100;i++) {
		l = (long)x;
		if (l <= (x+1) && l >= (x-1)) break;
		x *= 0.1;
		f *= 10.0;
	}

	if (i == 100) {
		/* yikes! the number is beyond what we can handle. What do we do? */
		(*iptr) = 0;
		return 0;
	}

	if (i != 0) {
		double i2;
		double ret;

		ret = my_modf(x0-l*f, &i2);
		(*iptr) = l*f + i2;
		return ret;
	} 

	(*iptr) = l;
	return x - (*iptr);
}

static void test1(double x)
{
	double i1, i2;
	double d1, d2;
	d1 = modf(x, &i1);
	d2 = my_modf(x, &i2);
	if (d1 != d2 || i1 != i2) {
		printf("%f\t%f\n", d1, i1);
		printf("%f\t%f\n", d2, i2);
	}
}

int main()
{
	test1(1.9);
	test1(164976598.8749875);
	test1(16497659895798297498763943987984.8749875);
	test1(-8734987047074075050509709874000789.1749875);
	test1(-16497659895798297498763943987984.8749875);
	test1(8734903083084098487047074075050509709874000789.1749875);
	return 0;
}
