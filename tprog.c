#include <time.h>
#include <stdio.h>

#define TM_YEAR_BASE 1900

/*******************************************************************
yield the difference between *A and *B, in seconds, ignoring leap seconds
********************************************************************/
static int tm_diff(struct tm *a, struct tm *b)
{
	int ay = a->tm_year + (TM_YEAR_BASE - 1);
	int by = b->tm_year + (TM_YEAR_BASE - 1);
	int intervening_leap_days =
		(ay/4 - by/4) - (ay/100 - by/100) + (ay/400 - by/400);
	int years = ay - by;
	int days = 365*years + intervening_leap_days + (a->tm_yday - b->tm_yday);
	int hours = 24*days + (a->tm_hour - b->tm_hour);
	int minutes = 60*hours + (a->tm_min - b->tm_min);
	int seconds = 60*minutes + (a->tm_sec - b->tm_sec);

	return seconds;
}

/*******************************************************************
  return the UTC offset in seconds west of UTC, or 0 if it cannot be determined
  ******************************************************************/
static int TimeZone(time_t t)
{
	struct tm *tm = gmtime(&t);
	struct tm tm_utc;
	if (!tm)
		return 0;
	tm_utc = *tm;
	tm = localtime(&t);
	if (!tm)
		return 0;
	return tm_diff(&tm_utc,tm);
}

time_t timegm2(struct tm *tm) 
{
	struct tm tm2, tm3;
	time_t t;

	tm2 = *tm;

	t = mktime(&tm2);
	tm3 = *localtime(&t);
	tm2 = *tm;
	tm2.tm_isdst = tm3.tm_isdst;
	t = mktime(&tm2);
	t -= TimeZone(t);

	return t;
}

main()
{
	struct tm tm;
	time_t tnow, t;

	tnow = time(NULL);

	for (t=tnow; t < tnow + (400 * 24 * 60 * 60); t += 600) {
		tm = *localtime(&t);

		if (timegm2(&tm) != timegm(&tm)) {
			printf("diff=%d at %s", 
			       timegm2(&tm) - timegm(&tm), 
			       asctime(&tm));
		}
	}
	return 0;
}
