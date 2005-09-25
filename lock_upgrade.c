#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

static int brlock(int fd, int type)
{
	struct flock lock;

	lock.l_type = type;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 1;
	lock.l_pid = 0;

	if (fcntl(fd,F_SETLKW,&lock) != 0) {
		printf("\nFailed to get lock type=%d in %d - %s\n",
		       type, (int)getpid(), strerror(errno));
		return -1;
	}
	return 0;
}

static int tlock_read(int fd)
{
	if (brlock(fd, F_RDLCK) != 0) return -1;
	sleep(2);
	if (brlock(fd, F_UNLCK) != 0) return -1;
	return 0;
}

static int tlock_write(int fd)
{
	if (brlock(fd, F_WRLCK) != 0) return -1;
	if (brlock(fd, F_UNLCK) != 0) return -1;
	return 0;
}

static int tlock_transaction(int fd)
{
	if (brlock(fd, F_RDLCK) != 0) return -1;
	if (brlock(fd, F_WRLCK) != 0) return -1;
	if (brlock(fd, F_UNLCK) != 0) return -1;
	return 0;
}

int main(void) 
{
	const char *fname = "upgradetest.dat";
	int fd, i;
	#define N 2
	pid_t pids[N];

	unlink(fname);
	fd = open(fname, O_CREAT|O_RDWR|O_EXCL, 0600);

	brlock(fd, F_RDLCK);

	/* fork 2 children */
	if ((pids[0]=fork()) == 0) {
		tlock_read(fd);
		exit(0);
	}
	if ((pids[0]=fork()) == 0) {
		tlock_write(fd);
		exit(0);
	}

	sleep(1);
	brlock(fd, F_UNLCK);

	tlock_transaction(fd);

	close(fd);

	for (i=0;i<N;i++) {
		waitpid(-1, NULL, 0);
	}

	exit(0);
}
