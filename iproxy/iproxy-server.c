#include "iproxy.h"

static void do_proxy_server(int fd_in)
{
	struct packet packet;
	size_t n;
	int fd_out;
	struct sockaddr saddr;
	socklen_t socklen;

	while (1) {
		bzero(&packet, sizeof(packet));
		socklen = sizeof(saddr);
		n = recvfrom(fd_in, &packet, sizeof(packet),
			     0, &saddr, &socklen);
		if (ntohl(packet.tag) != PROXY_TAG ||
		    ntohl(packet.length) != n) {
			continue;
		}

		fd_out = open_socket_out(SOCK_STREAM, "127.0.0.1", 
					 WEB_PORT);
					 

		if (fd_out == -1) {
			fprintf(stderr,"Can't contact web server!\n");
			continue;
		}

		write_all(fd_out, packet.buf, n - 12);

		while ((n = read(fd_out, packet.buf, 
				 sizeof(packet.buf))) > 0) {
			packet.length = htonl(n + 12);
			sendto(fd_in, &packet, ntohl(packet.length),
			       0, &saddr, socklen);
			packet.offset = htonl(ntohl(packet.offset) + n);
		}

		packet.length = htonl(12);
		sendto(fd_in, &packet, ntohl(packet.length),
		       0, &saddr, socklen);
		close(fd_out);
	}
}

int main(int argc, char *argv[])
{
	int fd;

	fd = open_socket_in(SOCK_DGRAM, "0.0.0.0", PROXY_PORT);
	if (fd == -1) {
		fprintf(stderr,"Failed to listen on %d\n", PROXY_PORT);
		exit(1);
	}

	do_proxy_server(fd);
	return 0;
}
