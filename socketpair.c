#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>

/* work out what fcntl flag to use for non-blocking */
#ifdef O_NONBLOCK
# define NONBLOCK_FLAG O_NONBLOCK
#elif defined(SYSV)
# define NONBLOCK_FLAG O_NDELAY
#else 
# define NONBLOCK_FLAG FNDELAY
#endif

#define socklen_t int

/****************************************************************************
Set a fd into nonblocking mode.
****************************************************************************/
int set_nonblocking(int fd)
{
	int val;

	if((val = fcntl(fd, F_GETFL, 0)) == -1)
		return -1;
	if (!(val & NONBLOCK_FLAG)) {
		val |= NONBLOCK_FLAG;
		fcntl(fd, F_SETFL, val);
	}
	return 0;
}

/****************************************************************************
Set a fd into blocking mode.
****************************************************************************/
int set_blocking(int fd)
{
	int val;

	if((val = fcntl(fd, F_GETFL, 0)) == -1)
		return -1;
	if (val & NONBLOCK_FLAG) {
		val &= ~NONBLOCK_FLAG;
		fcntl(fd, F_SETFL, val);
	}
	return 0;
}

int socketpair_tcp(int fd[2])
{
	int listener;
	struct sockaddr sock;
	socklen_t socklen = sizeof(sock);
	int len = socklen;
	int one = 1;
	int connect_done = 0;

	fd[0] = fd[1] = listener = -1;

	memset(&sock, 0, sizeof(sock));
	
	if ((listener = socket(PF_INET, SOCK_STREAM, 0)) == -1) goto failed;

	setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof(one));

	if (listen(listener, 1) != 0) goto failed;

	if (getsockname(listener, &sock, &socklen) != 0) goto failed;

	if ((fd[1] = socket(PF_INET, SOCK_STREAM, 0)) == -1) goto failed;

	setsockopt(fd[1],SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof(one));

	set_nonblocking(fd[1]);

	if (connect(fd[1],(struct sockaddr *)&sock,sizeof(sock)) == -1) {
		if (errno != EINPROGRESS) goto failed;
	} else {
		connect_done = 1;
	}

	if ((fd[0] = accept(listener, &sock, &len)) == -1) goto failed;

	setsockopt(fd[0],SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof(one));

	close(listener);
	if (connect_done == 0) {
		if (connect(fd[1],(struct sockaddr *)&sock,sizeof(sock)) != 0) goto failed;
	}

	set_blocking(fd[1]);

	/* all OK! */
	return 0;

 failed:
	if (fd[0] != -1) close(fd[0]);
	if (fd[1] != -1) close(fd[1]);
	if (listener != -1) close(listener);
	return -1;
}


main() { 
       int fd[2], ret;
       char buf[1024*1024];
       int maxfd;
       fd_set set;


       alarm(5);
       if (socketpair_tcp(fd) != 0) exit(1);

       FD_ZERO(&set);
       FD_SET(fd[0], &set);
       maxfd = fd[0] > fd[1] ? fd[0] : fd[1];
       select(maxfd+1, NULL, &set, NULL, NULL);
       ret = write(fd[0], buf, sizeof(buf));
       if (ret > 0 && ret < sizeof(buf)) exit(0);
       exit(1);
} 
