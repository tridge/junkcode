#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

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

#define LEN 0x1000
#define ADDR 0x0

int main()
{
	int fdpair[2];
	char buf[LEN];

	pipe(fdpair);
	if (fork()) {
		fd_set	fds;
		int no, ret=0;

		set_blocking(fdpair[0], 0);

		FD_ZERO(&fds);
		FD_SET(fdpair[0], &fds);

		no = select(32, &fds, NULL, NULL, NULL);
		
		if (no == 1) {
			ret = read(fdpair[0], buf, LEN);
			if (ret == 0) {
				fprintf(stderr,"Error: EOF on pipe\n");
				exit(1);
			}
		}
		printf("read %d bytes\n", ret);
		waitpid(-1, NULL, 0);
	} else {
		write(fdpair[1], buf, 0);
		sleep(1);
		write(fdpair[1], buf, LEN);
		_exit(0);
	}

	return 0;
}
