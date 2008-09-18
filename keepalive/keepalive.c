#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

static int am_server;
static int port = 4999;
static const char *host;

static int set_nonblocking(int fd)
{
	int val;
	if((val = fcntl(fd, F_GETFL, 0)) == -1)
		return -1;
	val |= O_NONBLOCK;
	return fcntl(fd, F_SETFL, val);
}

static void set_close_on_exec(int fd)
{
	unsigned v;
	v = fcntl(fd, F_GETFD, 0);
        fcntl(fd, F_SETFD, v | FD_CLOEXEC);
}

static void set_keepalive(int fd)
{
	int one = 1;
	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one));
}

static void set_nodelay(int fd)
{
	int one = 1;
	if (setsockopt(fd, SOL_TCP, TCP_NODELAY, &one, sizeof(one)) != 0) {
		perror("TCP_NODELAY");
		exit(1);
	}
}

static void client(void)
{
	int type = SOCK_STREAM;
	struct sockaddr_in sock_out;
	int fd;
	struct hostent *hp;  

	fd = socket(PF_INET, type, 0);
	if (fd == -1) {
		exit(1);
	}

	hp = gethostbyname(host);
	if (!hp) {
		printf("unknown host: %s\n", host);
		exit(1);
	}

	memcpy(&sock_out.sin_addr, hp->h_addr, hp->h_length);
	sock_out.sin_port = htons(port);
	sock_out.sin_family = PF_INET;

	if (connect(fd,(struct sockaddr *)&sock_out,sizeof(sock_out))) {
		close(fd);
		printf("failed to connect to %s - %s\n", 
			host, strerror(errno));
		exit(1);
	}

	set_nonblocking(fd);
	set_close_on_exec(fd);
	set_keepalive(fd);
	set_nodelay(fd);

	while (1) {
		fd_set	fds;
		struct timeval tv;
		int n;
		char buf[32];

		tv.tv_sec  = 5;
		tv.tv_usec = 0;

		FD_ZERO(&fds);
		FD_SET(fd, &fds);

		n = select(fd+1, &fds, NULL, NULL, &tv);
		if (n == -1) {
			perror("select");
			exit(1);
		}
		if (n == 1) {
			ssize_t num_ready = 0;
			if (ioctl(fd, FIONREAD, &num_ready) != 0) {
				perror("FIONREAD");
				exit(1);
			}
			if (num_ready == 0) {
				printf("got EOF!\n");
				exit(1);
			}
		}

		memset(buf, 1, 32);
		if (write(fd, buf, 32) != 32) {
			printf("write failed\n");
			exit(1);
		}
		printf("write ok\n");
		fflush(stdout);
	}
	
}

static void server(void)
{
	int num_connected=0;
	int fd[100];
	struct hostent *hp;
	struct sockaddr_in sock;
	char host_name[1000];
	int one=1;
	struct sockaddr addr;
	socklen_t in_addrlen = sizeof(addr);
	int res;

	/* get my host name */
	if (gethostname(host_name, sizeof(host_name)) == -1) { 
		printf("gethostname failed\n"); 
		exit(1);
	} 

	/* get host info */
	if ((hp = gethostbyname(host_name)) == 0) {
		printf("gethostbyname: Unknown host %s\n",host_name);
		exit(1);
	}
  
	bzero((char *)&sock,sizeof(sock));
	memcpy((char *)&sock.sin_addr,(char *)hp->h_addr, hp->h_length);

	sock.sin_port = htons( port );
	sock.sin_family = hp->h_addrtype;
	sock.sin_addr.s_addr = INADDR_ANY;
	res = socket(hp->h_addrtype, SOCK_STREAM, 0);
	if (res == -1) { 
		printf("socket failed\n"); 
		exit(1);
	}

	setsockopt(res,SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof(one));

	if (bind(res, (struct sockaddr * ) &sock,sizeof(sock)) < 0) { 
		perror("bind");
		exit(1);
	}

	if (listen(res, 5) == -1) {
		printf("listen failed\n");
		exit(1);
	}

	printf("Listening...\n");

	while (1) {
		fd_set	fds;
		struct timeval tv;
		int n, i;
		char buf[32];

		tv.tv_sec  = 5;
		tv.tv_usec = 0;

		FD_ZERO(&fds);
		FD_SET(res, &fds);
		for (i=0;i<num_connected;i++) {
			FD_SET(fd[i], &fds);
		}

		n = select(num_connected+4, &fds, NULL, NULL, &tv);
		if (n == -1) {
			perror("select");
			exit(1);
		}
		if (n == 0) continue;

		if (FD_ISSET(res, &fds)) {
			i=num_connected;
			fd[i] = accept(res,&addr,&in_addrlen);
			if (fd[i] == -1) {
				perror("accept");
				exit(1);
			}
			set_nonblocking(fd[i]);
			set_close_on_exec(fd[i]);
			set_keepalive(fd[i]);
			set_nodelay(fd[i]);

			num_connected++;
			printf("Connected fd=%d num_connected=%d\n", fd[i], num_connected);
		}
		for (i=0;i<num_connected;i++) {
			if (FD_ISSET(fd[i], &fds)) {
				ssize_t num_ready = 0;
				if (ioctl(fd[i], FIONREAD, &num_ready) != 0) {
					perror("FIONREAD");
					exit(1);
				}
				if (num_ready == 0) {
					printf("got EOF on fd %d\n", fd[i]);
					close(fd[i]);
					memmove(&fd[i], &fd[i+1], sizeof(fd[0]) * (num_connected-(i+1)));
					num_connected--;
					continue;
				}
				if (num_ready != 32) {
					printf("wrong read size\n");
				}
				if (read(fd[i], buf, 32) != 32) {
					printf("read failed\n");
				}
				printf("read on fd %d\n", fd[i]);
				fflush(stdout);
			}
		}
	}	
}

static void usage(void)
{
	printf("-p port\n-H host\n-S  (am server)");
}

int main(int argc, char *argv[])
{
	int opt;
	extern char *optarg;

	while ((opt = getopt (argc, argv, "p:H:hS")) != EOF) {
		switch (opt) {
		case 'p':
			port = strtol(optarg, NULL, 0);
			break;
		case 'S':
			am_server = 1;
			break;
		case 'H':
			host = optarg;
			break;
		case 'h':
		default:
			usage();
			exit(1);
		}
	}

	if (am_server) {
		server();
	} else {
		client();
	}

	return 0;
}
