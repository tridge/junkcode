/*
  a simple demonsration of a scaling problem in byte range locks on solaris

  try this as 'locker 800 1' and compare to 'locker 800 0'

  tridge@samba.org, July 2002
*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

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

static void slave(int fd, int nlocks)
{
	int i;
	for (i=1; i<nlocks+1; i++) {
		brlock(fd, i, F_RDLCK, F_SETLKW);
		usleep(1); /* essentially just a yield() */
	}

	/* this last lock will block until the master unlocks it */
	brlock(fd, 0, F_WRLCK, F_SETLKW);
	brlock(fd, 0, F_UNLCK, F_SETLKW);
}

int main(int argc, char *argv[])
{
	int i, fd;
	int nproc, nlocks;
	const char *fname = "locker.dat";
	char c;

	if (argc < 3) {
		printf("Usage: locker <nproc> <nlocks>\n");
		exit(1);
	}

	nproc = atoi(argv[1]);
	nlocks = atoi(argv[2]);
	setpgrp();

	fd = open(fname, O_RDWR | O_CREAT | O_TRUNC, 0600);
	if (fd == -1) {
		perror(fname);
		exit(1);
	}

	/* set the master lock - this stops the children exiting */
	brlock(fd, 0, F_WRLCK, F_SETLKW);

	for (i=0;i<nproc;i++) {
		switch (fork()) {
		case 0:
			slave(fd, nlocks);
			exit(0);
		case -1:
			perror("fork");
			kill(0, SIGTERM);
			exit(1);
		}
	}

	printf("hit enter to continue\n");
	read(0, &c, 1);

	printf("unlocking ....\n");
	brlock(fd, 0, F_UNLCK, F_SETLKW);

	while (waitpid(0, NULL, 0) > 0 || errno != ECHILD) /* noop */ ;

	printf("done\n");

	return 0;
}
