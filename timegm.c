#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

 time_t timegm(struct tm *tm) 
{
	time_t t = mktime(tm);
	t -= mktime(gmtime(&t)) - (int)mktime(localtime(&t));
	return t;
}

int get_time_zone(time_t t)
{
	struct tm *tm = gmtime(&t);
	struct tm tm_utc;
	if (!tm)
		return 0;
	tm_utc = *tm;
	tm = localtime(&t);
	if (!tm)
		return 0;
	return mktime(&tm_utc) - mktime(tm);
}

int main(void)
{
	time_t t = time(NULL);
	struct tm *tm_local = localtime(&t);
	struct tm *tm_gmt   = gmtime(&t);
	int diff;

	diff = timegm(tm_local) - (int)mktime(tm_local);

	printf("gmt1=%d\n", t);
	printf("loc1=%d\n", t + 36000);

	printf("loc2=%d\n", mktime(tm_local));
	printf("gmt2=%d\n", timegm(tm_local));

	printf("diff=%d\n", diff);

	printf("tz=%d\n", get_time_zone(t));
	return 0;
}
