#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

typedef uint64_t NTTIME;


static void test1(time_t t) { }

static void test2(NTTIME t) {}

static time_t test1_r(void) { return 0; }

static NTTIME test2_r(void) { return 0; }


int main(void)
{
	NTTIME nt = 0;
	time_t t = 0;

	test1(t);

	test1(nt);

	test2(t);

	test2(nt);

	t = test1_r();

	nt = test1_r();

	t = test2_r();

	nt = test2_r();

	return 0;
}
