/*
  this measures the ping-pong byte range lock latency. It is
  especially useful on a cluster of nodes sharing a common lock
  manager as it will give some indication of the lock managers
  performance under stress

  tridge@samba.org, February 2002

*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

static struct timeval tp1,tp2;

static void start_timer()
{
	gettimeofday(&tp1,NULL);
}

static double end_timer()
{
	gettimeofday(&tp2,NULL);
	return (tp2.tv_sec + (tp2.tv_usec*1.0e-6)) - 
		(tp1.tv_sec + (tp1.tv_usec*1.0e-6));
}

/* lock a byte range in a open file */
static int lock_range(int fd, int offset, int len)
{
	struct flock lock;

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = offset;
	lock.l_len = len;
	lock.l_pid = 0;
	
	return fcntl(fd,F_SETLKW,&lock);
}

/* unlock a byte range in a open file */
static int unlock_range(int fd, int offset, int len)
{
	struct flock lock;

	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = offset;
	lock.l_len = len;
	lock.l_pid = 0;
	
	return fcntl(fd,F_SETLKW,&lock);
}

/* run the ping pong test on fd */
static void ping_pong(int fd, int num_locks)
{
	unsigned count = 0;
	int i=0;

	start_timer();

	lock_range(fd, 0, 1);
	i = 0;

	while (1) {
		if (lock_range(fd, (i+1) % num_locks, 1) != 0) {
			printf("lock at %d failed! - %s\n",
			       (i+1) % num_locks, strerror(errno));
		}
		if (unlock_range(fd, i, 1) != 0) {
			printf("unlock at %d failed! - %s\n",
			       i, strerror(errno));
		}
		i = (i+1) % num_locks;
		count++;
		if (end_timer() > 1.0) {
			printf("%8u locks/sec\r", 
			       (unsigned)(2*count/end_timer()));
			fflush(stdout);
			start_timer();
			count=0;
		}
	}
}

int main(int argc, char *argv[])
{
	char *fname;
	int fd, num_locks;

	if (argc < 3) {
		printf("ping_pong <file> <num_locks>\n");
		exit(1);
	}

	fname = argv[1];
	num_locks = atoi(argv[2]);

	fd = open(fname, O_CREAT|O_RDWR, 0600);
	if (fd == -1) exit(1);

	ping_pong(fd, num_locks);

	return 0;
}
