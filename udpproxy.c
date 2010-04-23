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

static void get_address(const char *host, struct in_addr *addr)
{
       if (inet_pton(AF_INET, host, addr) <= 0) {
	       struct hostent *hp;
               hp = gethostbyname(host);
               if (!hp) {
                       fprintf(stderr,"unknown host %s\n", host);
		       exit(1);
               }
               memcpy(addr, hp->h_addr, hp->h_length);
       }
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

static void main_loop(int sock, const char *host1, const char *host2)
{
	unsigned char buf[10240];
	struct in_addr addr1, addr2;

	get_address(host1, &addr1);
	get_address(host2, &addr2);

	while (1) {
		fd_set fds;
		int ret;
		struct in_addr *addr;

		FD_ZERO(&fds);
		FD_SET(sock, &fds);

		ret = select(sock, &fds, NULL, NULL, NULL);
		if (ret == -1 && errno == EINTR) continue;
		if (ret <= 0) break;

		if (FD_ISSET(sock, &fds)) {
			static struct sockaddr_in from;
			static socklen_t fromlen = sizeof(from);
			int n = recvfrom(sock, buf, sizeof(buf), 0, 
					 (struct sockaddr *)&from, &fromlen);
			if (n <= 0) break;

			if (from.sin_addr.s_addr == addr1.s_addr) {
				addr = &addr2;
			} else if (from.sin_addr.s_addr == addr2.s_addr) {
				addr = &addr1;
			} else {
				printf("Unexpected packet from %s\n", inet_ntoa(from.sin_addr));
				continue;
			}

			from.sin_addr = *addr;
			ret = sendto(sock, buf, n, 0, (struct sockaddr *)&from, sizeof(from));
			if (ret != n) {
				printf("Failed to send %d bytes to %s - %d\n",
				       n, inet_ntoa(*addr), ret);
			}
		}
	}	
}

int main(int argc, char *argv[])
{
	int listen_port;
	char *host1, *host2;
	int sock_in;

	if (argc < 4) {
		printf("Usage: udpproxy <port> <host1> <host2>\n");
		exit(1);
	}

	listen_port = atoi(argv[1]);
	host1 = argv[2];
	host2 = argv[3];

	sock_in = open_socket_in(listen_port);
	if (sock_in == -1) {
		fprintf(stderr,"sock on port %d failed - %s\n", 
			listen_port, strerror(errno));
		exit(1);
	}

	main_loop(sock_in, host1, host2);

	return 0;
}
