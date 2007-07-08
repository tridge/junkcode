#include "ubench.h"

static int port=7001;
#define BUFSIZE 0x10000

static void server(int fd)
{
	char *buf;
	buf = malloc(BUFSIZE);

	while (1) {
		struct sockaddr from;
		socklen_t fromlen = sizeof(from);
		int n = recvfrom(fd, buf, BUFSIZE, 0, &from, &fromlen);
		int s1;
		if (n <= 0) break;
		s1 = ntohl(*(int *)buf);
		fromlen = sizeof(from);
		sendto(fd, buf, s1, 0, &from, fromlen);
	}

	exit(1);
}

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
		fprintf(stderr, "socket failed\n"); exit(1); 
	}

	setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof(one));

	/* now we've got a socket - we need to bind it */
	if (bind(fd, (struct sockaddr * ) &sock,sizeof(sock)) < 0) { 
		perror("bind");
		exit(1);
	}

	for (i=0;i<nservers;i++) {
		if (fork() == 0) server(fd);
	}
	printf("%d servers forked on port %d\n", nservers, port);
	pause();
}


static void usage(void)
{
	printf("\n"
"userver <options>\n"
"  -n nservers\n"
"  -p port\n");
}

int main(int argc, char *argv[])
{
	int opt;
	extern char *optarg;
	int nservers = 8;

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
