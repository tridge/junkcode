#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/*******************************************************************
find the difference in milliseconds between two struct timeval
values
********************************************************************/
int TvalDiff(struct timeval *tvalold,struct timeval *tvalnew)
{
  return((tvalnew->tv_sec - tvalold->tv_sec)*1000 + 
	 ((int)tvalnew->tv_usec - (int)tvalold->tv_usec)/1000);	 
}

/*******************************************************************
sleep for a specified number of milliseconds
********************************************************************/
void msleep(int t)
{
  int tdiff=0;
  struct timeval tval,t1,t2;  

  gettimeofday(&t1, NULL);
  gettimeofday(&t2, NULL);
  
  while (tdiff < t) {
    tval.tv_sec = (t-tdiff)/1000;
    tval.tv_usec = 1000*((t-tdiff)%1000);
 
    errno = 0;
    select(0,NULL,NULL, NULL, &tval);

    gettimeofday(&t2, NULL);
    tdiff = TvalDiff(&t1,&t2);
  }
}

/****************************************************************************
Set a fd into blocking/nonblocking mode. Uses POSIX O_NONBLOCK if available,
else
if SYSV use O_NDELAY
if BSD use FNDELAY
****************************************************************************/
int set_blocking(int fd, int set)
{
  int val;
#ifdef O_NONBLOCK
#define FLAG_TO_SET O_NONBLOCK
#else
#ifdef SYSV
#define FLAG_TO_SET O_NDELAY
#else /* BSD */
#define FLAG_TO_SET FNDELAY
#endif
#endif

  if((val = fcntl(fd, F_GETFL, 0)) == -1)
	return -1;
  if(set) /* Turn blocking on - ie. clear nonblock flag */
	val &= ~FLAG_TO_SET;
  else
    val |= FLAG_TO_SET;
  return fcntl( fd, F_SETFL, val);
#undef FLAG_TO_SET
}


int main()
{
	int fdpair[2];
	char buf[512] = {0,};

	pipe(fdpair);
	if (fork()) {
		fd_set fds;
		while (1) {
			int ret;

			FD_ZERO(&fds);
			FD_SET(fdpair[1], &fds);

			if (select(10, NULL, &fds, NULL, NULL) == 1) {
				printf("writing ..\n"); fflush(stdout);
				ret = write(fdpair[1], buf, sizeof(buf));
				printf("wrote %d\n", ret); fflush(stdout);
			}
		}
	} else {
		set_blocking(fdpair[0], 0);
		while (1) {
			int ret = read(fdpair[0], buf, 40);
			msleep(100);
		}
	}

	return 0;
}
