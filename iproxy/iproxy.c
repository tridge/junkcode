#include "iproxy.h"

static uint32 iproxy_port = PROXY_PORT;
static uint32 iproxy_tag = PROXY_TAG;

static void do_proxy(int sock)
{
	struct packet packet;
	size_t n = 0;
	int fd_out, fd_in;
	struct sockaddr saddr;
	struct sockaddr_in iaddr;
	socklen_t socklen;
	struct in_addr addr;
	int one = 1;
	off_t offset;

	if (iproxy_tag == IPROXY_LIST_TAG) {
		goto sendit;
	}

	listen(sock, 10);
	
	while (1) {
		fd_in = accept(sock, &saddr, &socklen);
		if (fork() == 0) break;
		close(fd_in);
	}
	close(sock);
	
	bzero(&packet, sizeof(packet));

	n = read(fd_in, &packet.buf, sizeof(packet.buf));
	
	printf("received %d byte request\n", n);

sendit:
	fd_out = open_socket_in(SOCK_DGRAM, ADDR_ANY, 0);

	setsockopt(fd_out,SOL_SOCKET,SO_BROADCAST,(char *)&one,sizeof(one));

	if (fd_out == -1) {
		fprintf(stderr,"Can't open broadcast socket\n");
		exit(1);
	}
	
	packet.tag = htonl(iproxy_tag);
	packet.length = htonl(n + 12);

	if (inet_pton(AF_INET, ADDR_BROADCAST, &addr) <= 0) {
		perror("inet_pton");
	}

	bzero(&iaddr, sizeof(iaddr));
	iaddr.sin_port = htons(iproxy_port);
	iaddr.sin_family = AF_INET;
	iaddr.sin_addr = addr;
	sendto(fd_out, &packet, ntohl(packet.length), 0,
	       (struct sockaddr *)&iaddr, sizeof(iaddr));

	printf("sent %d byte request\n", ntohl(packet.length));

	offset = 0;
	
	while ((n = read(fd_out, &packet, sizeof(packet))) > 0) {
		printf("received %d byte reply\n", ntohl(packet.length));

		if (ntohl(packet.length) != n ||
		    ntohl(packet.length) < 12) continue;

		if (ntohl(packet.tag) == IPROXY_LIST_TAG) {
			printf("%s", packet.buf);
			fflush(stdout);
			continue;
		}

		if (ntohl(packet.tag) != iproxy_tag) continue;

		if (ntohl(packet.length) == 12) break;

		if (ntohl(packet.offset) != offset) {
			printf("out of order packets!\n");
		}
		
		write_all(fd_in, packet.buf, ntohl(packet.length) - 12);
		bzero(&packet, sizeof(packet));

		offset += ntohl(packet.length)-12;
	}
	close(fd_out);

	exit(0);
}

static void usage(void)
{
	printf("
iproxy [options]
    -h            help
    -l            list servers
    -p PORT       port number
    -t TAG        tag number
");
	exit(1);
}


int main(int argc, char *argv[])
{
	int fd;
	int c;

	while ((c = getopt(argc, argv, "ht:p:l")) != -1) {
		switch (c) {
		case 't':
			iproxy_tag = atoi(optarg);
			break;
		case 'l':
			iproxy_tag = IPROXY_LIST_TAG;
			break;
		case 'p':
			iproxy_port = atoi(optarg);
			break;
		case 'h':
		default:
			usage();
			break;
		}
		
	}

	fd = open_socket_in(SOCK_STREAM, ADDR_ANY, iproxy_port);
	if (fd == -1) {
		fprintf(stderr,"Failed to listen on %d\n", iproxy_port);
		exit(1);
	}

	do_proxy(fd);
	return 0;
}
