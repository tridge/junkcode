#include "iproxy.h"

static uint32 iproxy_port = PROXY_PORT;
static uint32 iproxy_tag = PROXY_TAG;
static uint32 dest_port = WEB_PORT;

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

		printf("received request of size %d\n", n);

		if (ntohl(packet.length) != n) continue;

		if (ntohl(packet.tag) == IPROXY_LIST_TAG) {
			n = snprintf(packet.buf, sizeof(packet.buf),
				     "Hello from %d!\n", iproxy_tag);
			packet.length = htonl(n + 12);
			sendto(fd_in, &packet, ntohl(packet.length),
			       0, &saddr, socklen);
			continue;
		}

		if (ntohl(packet.tag) != iproxy_tag) continue;

		fd_out = open_socket_out(SOCK_STREAM, ADDR_LOCALHOST, 
					 dest_port);

		if (fd_out == -1) {
			fprintf(stderr,"Can't contact web server!\n");
			continue;
		}

		write_all(fd_out, packet.buf, n - 12);

		printf("sent request of size %d\n", n-12);

		while ((n = read(fd_out, packet.buf, 
				 sizeof(packet.buf))) > 0) {
			packet.length = htonl(n + 12);
			sendto(fd_in, &packet, ntohl(packet.length),
			       0, &saddr, socklen);
			printf("sent reply of size %d\n", n+12);
			packet.offset = htonl(ntohl(packet.offset) + n);
		}

		packet.length = htonl(12);
		sendto(fd_in, &packet, ntohl(packet.length),
		       0, &saddr, socklen);
		printf("sent reply end\n");
		close(fd_out);
	}
}

static void usage(void)
{
	printf("
iproxy-server [options]
    -h            help
    -p PORT       listen port number
    -d PORT       dest port
    -t TAG        tag number
");
	exit(1);
}

int main(int argc, char *argv[])
{
	int fd;
	int c;

	while ((c = getopt(argc, argv, "ht:p:d:")) != -1) {
		switch (c) {
		case 't':
			iproxy_tag = atoi(optarg);
			break;
		case 'p':
			iproxy_port = atoi(optarg);
			break;
		case 'd':
			dest_port = atoi(optarg);
			break;
		case 'h':
		default:
			usage();
			break;
		}
		
	}


	fd = open_socket_in(SOCK_DGRAM, ADDR_ANY, iproxy_port);
	if (fd == -1) {
		fprintf(stderr,"Failed to listen on %d\n", iproxy_port);
		exit(1);
	}

	do_proxy_server(fd);
	return 0;
}
