#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>

/* work out what fcntl flag to use for non-blocking */
#ifdef O_NONBLOCK
# define NONBLOCK_FLAG O_NONBLOCK
#elif defined(SYSV)
# define NONBLOCK_FLAG O_NDELAY
#else 
# define NONBLOCK_FLAG FNDELAY
#endif

static int fdpair_pipe(int fd[2])
{
	return pipe(fd);
}

static int fdpair_socketpair(int fd[2])
{
	return socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
}

/****************************************************************************
Set a fd into nonblocking mode.
****************************************************************************/
static int set_nonblocking(int fd)
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
static int set_blocking(int fd)
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

static int fdpair_tcp(int fd[2])
{
	int listener;
	struct sockaddr sock;
	int socklen = sizeof(sock);
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


static void testfn(char *name, int (*fn)(int fd[2]))
{
	int dfile;
	int fd[2];
	char *fname = "sockbug.tmp";
	int count;
	pid_t pid;
	char c = 'x';
	struct stat st;
	signal(SIGCHLD, SIG_IGN);

	if (fn(fd) != 0) {
		perror(name);
		exit(1);
	}

	if ((pid=fork()) == 0) {
		/* we will put the data in a file as well as in the socketpair */
		dfile = open(fname, O_WRONLY|O_CREAT|O_TRUNC);
		if (dfile == -1) {
			perror(fname);
			exit(1);
		}
	
		close(fd[0]);

		set_nonblocking(fd[1]);

		while (write(fd[1], &c, 1) == 1) 
			write(dfile, &c, 1);
		close(dfile);
		exit(1);
	}

	close(fd[1]);
	waitpid(pid, NULL, 0);
	
	count=0;
	while (read(fd[0], &c, 1) == 1) count++;

	stat(fname, &st);
	printf("%s: count was %d. Should have been %d\n", 
	       name, count, (int)st.st_size);

	unlink(fname);
}

int main(int argc, char *argv[])
{
	testfn("tcp", fdpair_tcp);
	testfn("pipe", fdpair_pipe);
	testfn("socketpair", fdpair_socketpair);
	return 0;
}
