#include "iproxy.h"

static void do_proxy(int sock)
{
	struct packet packet;
	size_t n;
	int fd_out, fd_in;
	struct sockaddr saddr;
	struct sockaddr_in iaddr;
	socklen_t socklen;
	struct in_addr addr;
	int one = 1;

	listen(sock, 10);
	
	while (1) {
		fd_in = accept(sock, &saddr, &socklen);
		if (fork() == 0) break;
		close(fd_in);
	}
	close(sock);
	
	bzero(&packet, sizeof(packet));

	n = read(fd_in, &packet.buf, sizeof(packet.buf));

	fd_out = open_socket_in(SOCK_DGRAM, "0.0.0.0", PROXY_PORT+1);

	setsockopt(fd_out,SOL_SOCKET,SO_BROADCAST,(char *)&one,sizeof(one));

	if (fd_out == -1) {
		fprintf(stderr,"Can't open broadcast socket\n");
		exit(1);
	}
	
	packet.tag = htonl(PROXY_TAG);
	packet.length = htonl(n + 12);

	if (inet_pton(AF_INET, "255.255.255.255", &addr) <= 0) {
		perror("inet_pton");
	}

	bzero(&iaddr, sizeof(iaddr));
	iaddr.sin_port = htons(PROXY_PORT);
	iaddr.sin_family = AF_INET;
	iaddr.sin_addr = addr;
	sendto(fd_out, &packet, ntohl(packet.length), 0,
	       (struct sockaddr *)&iaddr, sizeof(iaddr));
	
	while ((n = read(fd_out, &packet, sizeof(packet))) > 0) {
		if (ntohl(packet.tag) != PROXY_TAG ||
		    ntohl(packet.length) != n ||
		    ntohl(packet.length) < 12) continue;

		if (ntohl(packet.length) == 12) break;
		
		write_all(fd_in, packet.buf, ntohl(packet.length) - 12);
		bzero(&packet, sizeof(packet));
	}
	close(fd_out);

	exit(0);
}

int main(int argc, char *argv[])
{
	int fd;

	fd = open_socket_in(SOCK_STREAM, "0.0.0.0", PROXY_PORT);
	if (fd == -1) {
		fprintf(stderr,"Failed to listen on %d\n", PROXY_PORT);
		exit(1);
	}

	do_proxy(fd);
	return 0;
}
