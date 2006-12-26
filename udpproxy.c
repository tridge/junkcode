#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>

#define MAX(a,b) ((a)>(b)?(a):(b))

/* open a socket to a remote host with the specified port */
static int open_socket_out(const char *host, int port)
{
	struct sockaddr_in sock_out;
	int res;
	struct hostent *hp;  
	struct in_addr addr;

	res = socket(PF_INET, SOCK_DGRAM, 0);
	if (res == -1) {
		return -1;
	}

	if (inet_pton(AF_INET, host, &addr) > 0) {
		memcpy(&sock_out.sin_addr, &addr, sizeof(addr));
	} else {
		hp = gethostbyname(host);
		if (!hp) {
			fprintf(stderr,"unknown host %s\n", host);
			return -1;
		}
		memcpy(&sock_out.sin_addr, hp->h_addr, hp->h_length);
	}

	sock_out.sin_port = htons(port);
	sock_out.sin_family = PF_INET;

	if (connect(res,(struct sockaddr *)&sock_out,sizeof(sock_out)) != 0) {
		close(res);
		fprintf(stderr,"failed to connect to %s (%s)\n", 
			host, strerror(errno));
		return -1;
	}

	return res;
}


/*
  open a socket of the specified type, port and address for incoming data
*/
int open_socket_in(int port)
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

	res = socket(AF_INET, SOCK_DGRAM, 0);
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
	usleep(1000);
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

static void main_loop(int sock1, int sock2)
{
	unsigned char buf[10240];
	int i=0;
	struct sockaddr from;
	socklen_t fromlen = sizeof(from);
	int connected = 0;

	while (1) {
		fd_set fds;
		int ret;

		FD_ZERO(&fds);
		FD_SET(sock1, &fds);
		FD_SET(sock2, &fds);

		ret = select(MAX(sock1, sock2)+1, &fds, NULL, NULL, NULL);
		if (ret == -1 && errno == EINTR) continue;
		if (ret <= 0) break;

		if (FD_ISSET(sock1, &fds)) {
			int n = recvfrom(sock1, buf, sizeof(buf), 0, &from, &fromlen);
			if (n <= 0) break;

			if (!connected) {
				connect(sock1,&from,sizeof(from));
				connected = 1;
			}

//			printf("out %d bytes\n", n);
			write_all(sock2, buf, n);
		}

		if (FD_ISSET(sock2, &fds)) {
			int n = read(sock2, buf, sizeof(buf));
			if (n <= 0) break;

//			printf("in %d bytes\n", n);
			write_all(sock1, buf, n);
		}
	}	
}

static int sig_alrm(int sig)
{
	return 0;
}

int main(int argc, char *argv[])
{
	int listen_port, dest_port;
	char *host;
	int sock_in;
	int sock_out;
	struct sockaddr addr;
	int in_addrlen = sizeof(addr);
	char buf[8192];

	if (argc < 4) {
		printf("Usage: sockspy <inport> <host> <port>\n");
		exit(1);
	}

	listen_port = atoi(argv[1]);
	host = argv[2];
	dest_port = atoi(argv[3]);

	sock_in = open_socket_in(listen_port);

	if (sock_in == -1) {
		fprintf(stderr,"sock on port %d failed - %s\n", 
			listen_port, strerror(errno));
		exit(1);
	}

	signal(SIGCHLD, SIG_IGN);

	signal(SIGALRM, sig_alrm);

	sock_out = open_socket_out(host, dest_port);

	main_loop(sock_in, sock_out);

	return 0;
}
