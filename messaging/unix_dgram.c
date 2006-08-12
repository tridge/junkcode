/* 
   measure latency of unix domain sockets
   tridge@samba.org July 2006
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>


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

static void fatal(const char *why)
{
	fprintf(stderr, "fatal: %s - %s\n", why, strerror(errno));
	exit(1);
}

/*
  connect to a unix domain socket
*/
int ux_socket_connect(const char *name)
{
	int fd;
        struct sockaddr_un addr;

        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, name, sizeof(addr.sun_path));

	fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (fd == -1) {
		return -1;
	}
	
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		close(fd);
		return -1;
	}

	return fd;
}


/*
  create a unix domain socket and bind it
  return a file descriptor open on the socket 
*/
static int ux_socket_bind(const char *name)
{
	int fd;
        struct sockaddr_un addr;

	/* get rid of any old socket */
	unlink(name);

	fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (fd == -1) return -1;

        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, name, sizeof(addr.sun_path));

        if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		close(fd);
		return -1;
	}	

	return fd;
}

static void worker(const char *n1, const char *n2, int w)
{
	int s1 = ux_socket_bind(n1);
	int s2;
	int count=0;

	sleep(1);

	s2 = ux_socket_connect(n2);
	start_timer();

	while (1) {
		char c=0;
		if (write(s2, &c, 1) != 1) {
			fatal("write");
		}
		if (read(s1, &c, 1) != 1) {
			fatal("read");
		}
		if (w == 1 && (end_timer() > 1.0)) {
			printf("%8u ops/sec\r", 
			       (unsigned)(2*count/end_timer()));
			fflush(stdout);
			start_timer();
			count=0;
		}
		count++;
	}
}

int main(int argc, char *argv[])
{
	if (fork() == 0) {
		worker("sock2.unx", "sock1.unx", 0);
	}
	worker("sock1.unx", "sock2.unx", 1);
	return 0;
}
