#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

typedef uint64_t NTTIME;

typedef struct {
	uint64_t t;
} NTTIME_s;


static void test1(time_t t) { }

static void test2(NTTIME t) {}

static void test3(NTTIME_s t) {}

static time_t test1_r(void) { return 0; }

static NTTIME test2_r(void) { return 0; }

static NTTIME_s test3_r(void) { NTTIME_s t = { 0 }; return t; }



int main(void)
{
	NTTIME nt = 0;
	NTTIME_s nts = { 0 };
	time_t t = 0;

	test1(t);
	test1(nt);
	test1(nts);

	test2(t);
	test2(nt);
	test2(nts);

	t   = test1_r();
	nt  = test1_r();
	nts = test1_r();

	t   = test2_r();
	nt  = test2_r();
	nts = test2_r();

	t   = test3_r();
	nt  = test3_r();
	nts = test3_r();

	return 0;
}
