#include <stdlib.h>
#include <stdio.h>

main()
{
	int s_a, s_b;
	unsigned u_a, u_b;
	size_t a, b;
	size_t r;
	s_a = u_a = a = (1<<30) + 5;
	s_b = u_b = b = 5000;

	r = s_a * s_b;
	printf("r=%u\n", r);

	r = u_a * s_b;
	printf("r=%u\n", r);

	r = s_a * u_b;
	printf("r=%u\n", r);

	r = u_a * u_b;
	printf("r=%u\n", r);
}
