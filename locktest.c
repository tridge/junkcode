#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/wait.h>

#define FNAME "locktest.dat"

#ifdef GLIBC_HACK_FCNTL64
/* this is a gross hack. 64 bit locking is completely screwed up on
   i386 Linux in glibc 2.1.95 (which ships with RedHat 7.0). This hack
   "fixes" the problem with the current 2.4.0test kernels 
*/

#define fcntl fcntl64
#undef F_SETLKW 
#undef F_SETLK 
#define F_SETLK 13
#define F_SETLKW 14

#include <asm/unistd.h>
int fcntl64(int fd, int cmd, struct flock * lock)
{
	return syscall(__NR_fcntl64, fd, cmd, lock);
}
#endif /* HACK_FCNTL64 */


static pid_t child_pid;

typedef unsigned long long ull;

static int brlock(int fd, off_t offset, size_t len, int set, int rw_type, int lck_type)
{
	struct flock fl;

	fl.l_type = set?rw_type:F_UNLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = offset;
	fl.l_len = len;
	fl.l_pid = 0;

	if (fcntl(fd, lck_type, &fl) != 0) {
		return -1;
	}
	return 0;
}

static void child_main(int pfd)
{
	int fd;
	off_t ofs;
	size_t len;
	int ret;
	char line[1000];
	struct stat st;

	while ((fd = open(FNAME,O_RDWR)) == -1) sleep(1);

	while (1) {
		read(pfd, &ofs, sizeof(ofs));
		read(pfd, &len, sizeof(len));

		/* check that we can't lock that range */
		if (brlock(fd, ofs, len, 1, F_WRLCK, F_SETLK) == 0) {
			printf("ERROR: able to lock range %lld:%lld\n", (ull)ofs, (ull)len);
			goto failed;
		}
		
		/* but we should be able to lock just before and after it */
		if (brlock(fd, ofs+len, 1, 1, F_WRLCK, F_SETLK) == -1) {
			printf("ERROR: unable to lock range %lld:%lld\n", (ull)ofs+len, (ull)1);
			goto failed;
		}
		if (brlock(fd, ofs-1, 1, 1, F_WRLCK, F_SETLK) == -1) {
			printf("ERROR: unable to lock range %lld:%lld\n", (ull)ofs-1, (ull)1);
			goto failed;
		}

		/* and remove them again */
		if (brlock(fd, ofs+len, 1, 0, F_WRLCK, F_SETLK) == -1) {
			printf("ERROR: unable to unlock range %lld:%lld\n", (ull)ofs+len, (ull)1);
			goto failed;
		}
		if (brlock(fd, ofs-1, 1, 0, F_WRLCK, F_SETLK) == -1) {
			printf("ERROR: unable to unlock range %lld:%lld\n", (ull)ofs-1, (ull)1);
			goto failed;
		}
		
		/* seems OK */
		ret = 0;
		write(pfd, &ret, sizeof(ret));
	}

 failed:

	fstat(fd, &st);
	snprintf(line, sizeof(line), "egrep POSIX.*%u /proc/locks", (int)st.st_ino);
	system(line);

	ret = -1;
	write(pfd, &ret, sizeof(ret));
	sleep(10);
	exit(1);
}

static void parent_main(int pfd)
{
	int fd = open(FNAME,O_RDWR|O_CREAT|O_TRUNC, 0600);
	off_t ofs;
	size_t len;
	int ret, i;
	struct {
		off_t ofs;
		size_t len;
	} tests[] = {
		{7, 1},
		{1<<30, 1<<25},
#if (_FILE_OFFSET_BITS == 64)
		{1LL<<40, 1<<10},
		{1LL<<61, 1<<30},
		{(1LL<<62) - 10, 1},
#endif
	};


	if (fd == -1) {
		perror(FNAME);
		goto failed;
	}

	for (i=0;i<sizeof(tests)/sizeof(tests[0]); i++) {
		ofs = tests[i].ofs;
		len = tests[i].len;

		if (brlock(fd, ofs, len, 1, F_WRLCK, F_SETLK) == -1) {
			printf("lock 1 failed\n");
			goto failed;
		}

		write(pfd, &ofs, sizeof(ofs));
		write(pfd, &len, sizeof(len));

		read(pfd, &ret, sizeof(ret));
		if (ret != 0) {
			printf("child reported failure\n");
			goto failed;
		}

		printf("test %d OK\n", i);
	}
	printf("all tests OK\n");
	kill(child_pid, SIGKILL);
	waitpid(child_pid, NULL, 0);
	return;

 failed:
	kill(child_pid, SIGKILL);
	waitpid(child_pid, NULL, 0);
	exit(1);
}


int main(int argc, char *argv[])
{
	int pfd[2];
	struct flock fl;

	socketpair(AF_UNIX, SOCK_STREAM, 0, pfd);

        printf("sizeof(off_t) : %d\n", sizeof(off_t));
        printf("sizeof(size_t) : %d\n", sizeof(size_t));
        printf("sizeof(l_start) : %d\n", sizeof(fl.l_start));

	if ((child_pid=fork()) == 0) {
		close(pfd[0]);
		child_main(pfd[1]);
	} else {
		close(pfd[1]);
		parent_main(pfd[0]);
	}

	return 0;
}
