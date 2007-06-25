#include <math.h>

main()
{
	int i;
	double x;
	long long y;

	y = x = 1;

	for (i=0;i<64;i++) {
		x = (x * 2.0);
		x++;
		y = (y * 2.0);
		y++;
		printf("%d %.0f %lld %llx %g %g\n", i, x, y, y, x/(10.0*1000*1000*60*60*24*365), log10(x));
	}
}
