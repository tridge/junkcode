/*
  listen on a TCP port. When a connection comes in, open a local character devices 
  (such as a serial port) and send/recv data between the tcp socket and the serial
  device

  tridge@samba.org, March 2004
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>

static void set_nonblocking(int fd)
{
	unsigned v = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, v | O_NONBLOCK);
}

/*
  open a socket of the specified type, port and address for incoming data
*/
static int open_socket_in(int port)
{
	struct sockaddr_in sock;
	int res;
	int one=1;

	memset(&sock,0,sizeof(sock));

#ifdef HAVE_SOCK_SIN_LEN
	sock.sin_len = sizeof(sock);
#endif
	sock.sin_port = htons(port);
	sock.sin_family = AF_INET;

	res = socket(AF_INET, SOCK_STREAM, 0);
	if (res == -1) { 
		fprintf(stderr, "socket failed\n"); return -1; 
		return -1;
	}

	setsockopt(res,SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof(one));

	if (bind(res, (struct sockaddr *)&sock, sizeof(sock)) < 0) { 
		return(-1); 
	}

	return res;
}

/* write to a file descriptor, making sure we get all the data out or
 * die trying */
static void write_all(int fd, unsigned char *s, size_t n)
{
	while (n) {
		int r;
		r = write(fd, s, n);
		if (r <= 0) {
			exit(1);
		}
		s += r;
		n -= r;
	}
}

#define MAX(a,b) ((a)>(b)?(a):(b))

static void main_loop(int fd1, int fd2)
{
	unsigned char buf[1024];

	set_nonblocking(fd1);
	set_nonblocking(fd2);

	while (1) {
		fd_set fds;
		int ret;

		FD_ZERO(&fds);
		FD_SET(fd1, &fds);
		FD_SET(fd2, &fds);

		ret = select(MAX(fd1, fd2)+1, &fds, NULL, NULL, NULL);
		if (ret == -1 && errno == EINTR) continue;
		if (ret <= 0) break;

		if (FD_ISSET(fd1, &fds)) {
			int n = read(fd1, buf, sizeof(buf));
			if (n <= 0) break;
			write_all(fd2, buf, n);
		}

		if (FD_ISSET(fd2, &fds)) {
			int n = read(fd2, buf, sizeof(buf));
			if (n <= 0) break;
			write_all(fd1, buf, n);
		}
	}	
}


static int open_serial(const char *device)
{
	struct termios tty;
	int fd;
	char *cmd;

	fd = open(device, O_RDWR);
	if (fd == -1) {
		return -1;
	}
	
	asprintf(&cmd, "stty 5:4:cbe:a30:3:1c:7f:15:4:0:1:0:11:13:1a:0:12:f:17:16:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0 < %s", device);
	system(cmd);
	free(cmd);

	if (ioctl(fd,TCGETS,&tty) != 0) {
		close(fd);
		return -1;
	}

	tty.c_cflag &= ~CBAUD;
	tty.c_cflag |= B19200;

	if (ioctl(fd,TCSETS,&tty) != 0) {
		close(fd);
		return -1;
	}

	return fd;
}

int main(int argc, char *argv[])
{
	int listen_port;
	int sock_in;
	int fd_out;
	int listen_fd;
	struct sockaddr addr;
	int in_addrlen = sizeof(addr);
	const char *device;

	if (argc < 3) {
		printf("Usage: serialtcp <inport> <device>\n");
		exit(1);
	}

	listen_port = atoi(argv[1]);
	device = argv[2];

	signal(SIGCHLD, SIG_IGN);

	listen_fd = open_socket_in(listen_port);

	if (listen_fd == -1) {
		fprintf(stderr,"listen on port %d failed - %s\n", 
			listen_port, strerror(errno));
		exit(1);
	}

	if (listen(listen_fd, 5) == -1) {
		fprintf(stderr,"listen failed\n");
		exit(1);
	}

	while ((sock_in = accept(listen_fd,&addr,&in_addrlen)) != -1) {
		if (fork() == 0) {
			close(listen_fd);

			fd_out = open_serial(device);
			
			if (fd_out == -1) {
				perror(device);
				exit(1);
			}

			main_loop(sock_in, fd_out);
			exit(0);
		}

		close(sock_in);
	}

	return 0;
}
