/* a simple distributed lock manager server

   Andrew Tridgell (tridge@samba.org), February 2003

*/

#include "dl_server.h"

static int port = DL_PORT;


static void server(int fd)
{
	char *buf;
	size_t bufsize = 0x1000;
	uint32 count = 0;

	/* a simple static packet buffer */
	buf = malloc(bufsize);

	start_timer();

	while (1) {
		struct sockaddr from;
		struct sockaddr_in *from_in;
		socklen_t fromlen = sizeof(from);
		int request_length;
		double t;

		from_in = (struct sockaddr_in *)&from;

		/* wait for a packet */
		request_length = recvfrom(fd, buf, bufsize, 0, &from, &fromlen);
		if (request_length <= 0) break;

		/* assume its an IP protocol */
		from_in = (struct sockaddr_in *)&from;

		/* process the request */
		process_packet(buf, request_length, fd, from_in);

		count++;
		t = end_timer();
		if (t > 1.0) {
			printf("%8u locks/sec\r", (unsigned)(count/t));
			fflush(stdout);
			start_timer();
			count=0;
		}
	}

	exit(1);
}

/* bind to the socket and launch the main server loops */
static void listener(int nservers)
{
	struct sockaddr_in sock;
	int fd;
	int one=1;
	int i;

	bzero((char *)&sock,sizeof(sock));
	sock.sin_port = htons(port);
	sock.sin_family = AF_INET;
	sock.sin_addr.s_addr = 0;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1) { 
		fatal("failed to create socket - %s", strerror(errno));
	}

	setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof(one));

	/* now we've got a socket - we need to bind it */
	if (bind(fd, (struct sockaddr * ) &sock,sizeof(sock)) < 0) { 
		fatal("failed to bind - %s", strerror(errno));
		exit(1);
	}

	for (i=0;i<nservers;i++) {
		if (fork() == 0) server(fd);
	}
	printf("%d servers forked on port %d\n", nservers, port);
	pause();
}


/* show some basic help */
static void usage(void)
{
	printf("\n"
"dl_server <options>\n"
"  -n nservers\n"
"  -p port\n");
}

int main(int argc, char *argv[])
{
	int opt;
	extern char *optarg;
	int nservers = 1;

	while ((opt = getopt (argc, argv, "hn:p:")) != EOF) {
		switch (opt) {
		case 'n':
			nservers = atoi(optarg);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'h':
		default:
			usage();
			exit(1);
		}
	}

	listener(nservers);

	return 0;
}
