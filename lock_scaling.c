/*
  demonstrate problem with linear lock lists

  tridge@samba.org, February 2004
*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>


static struct timeval tp1,tp2;

static double tvdiff(struct timeval *tv1, struct timeval *tv2)
{
	return (tv2->tv_sec + (tv2->tv_usec*1.0e-6)) - 
		(tv1->tv_sec + (tv1->tv_usec*1.0e-6));
}

static void start_timer()
{
	gettimeofday(&tp1,NULL);
}

static double end_timer()
{
	gettimeofday(&tp2,NULL);
	fflush(stdout);
	return tvdiff(&tp1, &tp2);
}

static int brlock(int fd, off_t offset, int rw_type, int lck_type)
{
	struct flock fl;
	int ret;

	fl.l_type = rw_type;
	fl.l_whence = SEEK_SET;
	fl.l_start = offset;
	fl.l_len = 1;
	fl.l_pid = 0;

	ret = fcntl(fd,lck_type,&fl);

	if (ret == -1) {
		printf("brlock failed offset=%d rw_type=%d lck_type=%d : %s\n", 
		       (int)offset, rw_type, lck_type, strerror(errno));
	}
	return 0;
}


int main(int argc, char *argv[])
{
	int i, fd;
	int nlocks;
	const char *fname = "lock_scaling.dat";

	if (argc < 2) {
		printf("Usage: lock_scaling <nlocks>\n");
		exit(1);
	}

	nlocks = atoi(argv[1]);

	fd = open(fname, O_RDWR | O_CREAT | O_TRUNC, 0600);
	if (fd == -1) {
		perror(fname);
		exit(1);
	}

	for (i=0;i<nlocks;i++) {
		brlock(fd, 2*i, F_WRLCK, F_SETLKW);
	}

	while (1) {
		int count=0;
		double t;
		start_timer();

		while ((t=end_timer()) < 1.0) {
			brlock(fd, 2*nlocks, F_WRLCK, F_SETLKW);
			brlock(fd, 2*nlocks, F_UNLCK, F_SETLKW);
			count++;
		}
		printf("%.1f locks/sec\r", count/t);
		fflush(stdout);
	}


	return 0;
}
