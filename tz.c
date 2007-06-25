#include <stdlib.h>
#include <stdio.h>
#include <time.h>

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

  printf("tm_diff ay=%d by=%d ild=%d y=%d d=%d h=%d m=%d s=%d am=%d bm=%d asec=%d bsec=%d\n",
	   ay, by, intervening_leap_days, years, days, hours, minutes, seconds,
	   a->tm_min, b->tm_min,
	   a->tm_sec, b->tm_sec
	   );

  if (a->tm_sec != b->tm_sec)
	  printf("Your timezone file is broken\n");
  return seconds;
}

/*******************************************************************
  return the UTC offset in seconds west of UTC
  ******************************************************************/
static int TimeZone(time_t t)
{
  struct tm tm_utc = *(gmtime(&t));
  return tm_diff(&tm_utc,localtime(&t));
}

int main(int argc, char *argv[])
{
	TimeZone(time(NULL));
	return 0;
}
