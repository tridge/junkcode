#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define BOOL int

/****************************************************************************
Set a fd into blocking/nonblocking mode. Uses POSIX O_NONBLOCK if available,
else
if SYSV use O_NDELAY
if BSD use FNDELAY
****************************************************************************/
int set_blocking(int fd, BOOL set)
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

/*******************************************************************
this is like socketpair but uses tcp. It is used by the Samba
regression test code
The function guarantees that nobody else can attach to the socket,
or if they do that this function fails and the socket gets closed
returns 0 on success, -1 on failure
the resulting file descriptors are symmetrical
 ******************************************************************/
static int socketpair_tcp(int fd[2])
{
	int listener;
	struct sockaddr sock;
	struct sockaddr_in sock2;
	socklen_t socklen = sizeof(sock);
	int connect_done = 0;
	
	fd[0] = fd[1] = listener = -1;

	memset(&sock, 0, sizeof(sock));
	
	if ((listener = socket(PF_INET, SOCK_STREAM, 0)) == -1) goto failed;

        memset(&sock2, 0, sizeof(sock2));
#ifdef HAVE_SOCK_SIN_LEN
        sock2.sin_len = sizeof(sock2);
#endif
        sock2.sin_family = PF_INET;

        bind(listener, (struct sockaddr *)&sock2, sizeof(sock2));

	if (listen(listener, 1) != 0) goto failed;

	if (getsockname(listener, &sock, &socklen) != 0) goto failed;

	if ((fd[1] = socket(PF_INET, SOCK_STREAM, 0)) == -1) goto failed;

	set_blocking(fd[1], 0);

	if (connect(fd[1],(struct sockaddr *)&sock,sizeof(sock)) == -1) {
		if (errno != EINPROGRESS) goto failed;
	} else {
		connect_done = 1;
	}

	if ((fd[0] = accept(listener, &sock, &socklen)) == -1) goto failed;

	close(listener);
	if (connect_done == 0) {
		if (connect(fd[1],(struct sockaddr *)&sock,sizeof(sock)) != 0
		    && errno != EISCONN) goto failed;
	}

	set_blocking(fd[1], 1);

	/* all OK! */
	return 0;

 failed:
	if (fd[0] != -1) close(fd[0]);
	if (fd[1] != -1) close(fd[1]);
	if (listener != -1) close(listener);
	return -1;
}


/*******************************************************************
run a program on a local tcp socket, this is used to launch smbd
when regression testing
the return value is a socket which is attached to a subprocess
running "prog". stdin and stdout are attached. stderr is left
attached to the original stderr
 ******************************************************************/
int sock_exec(const char *prog)
{
	int fd[2];
	if (socketpair_tcp(fd) != 0) return -1;
	if (fork() == 0) {
		close(fd[0]);
		close(0);
		close(1);
		dup(fd[1]);
		dup(fd[1]);
		exit(system(prog));
	}
	close(fd[1]);
	return fd[0];
}


int main(int argc, char *argv[])
{
	int fd;
	char buf[100];
	int n;

	fd = sock_exec(argv[1]);

	while ((n = read(fd, buf, sizeof(buf))) > 0) {
		write(1, buf, n);
	}
	return 0;
}
