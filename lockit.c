/* run a command with a lock held
   tridge@samba.org, April 2002
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

static void usage(void)
{
	printf("
lockit <lockfile> <command>

Runs a command with a lockfile held. If the lock is already held then blocks
waiting for the lock file to be released before continuing.

Note that after running the lockfile is left behind in the filesystem. This is 
correct behaviour.

The lock is inherited across exec but not fork
");
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

int main(int argc, char *argv[])
{
	char *lockfile;
	int fd;
	if (argc < 3) {
		usage();
		exit(1);
	}

	lockfile = argv[1];

	fd = open(lockfile, O_CREAT|O_RDWR, 0644);
	if (fd == -1) {
		perror(lockfile);
		exit(1);
	}

	if (lock_range(fd, 0, 1) != 0) {
		perror(lockfile);
		exit(2);
	}

	return execvp(argv[2], argv+2);
}
