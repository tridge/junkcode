#include "socklib.h"


static char *tcp_options="";
static int port=7001;
static int bufsize=8192;

static void server(int fd)
{
	char *buf;
	int total=0;

	signal(SIGPIPE, SIG_IGN);
	
	set_socket_options(fd, tcp_options);

	buf = (char *)malloc(bufsize);
	if (!buf) {
		fprintf(stderr,"out of memory\n");
		exit(1);
	}

	memset(buf, 'Z', bufsize);

	start_timer();

	while (1) {
		int ret = write(fd, buf, bufsize);
		if (ret <= 0) break;
		total += ret;
		if (end_timer() > 2.0) {
			report_time(total);
			total = 0;
			start_timer();
		}
	}
	report_time(total);

	exit(0);
}

static void listener(void)
{
	int sock;
	
	sock = open_socket_in(SOCK_STREAM, port, INADDR_ANY);

	if (listen(sock, 5) == -1) {
		fprintf(stderr,"listen failed\n");
		exit(1);
	}

	while (1) {
		struct sockaddr addr;
		int in_addrlen = sizeof(addr);
		int fd;

		printf("waiting for connection\n");

		fd = accept(sock,&addr,&in_addrlen);

		if (fd != -1) {
			printf("accepted\n");
			if (fork() == 0) server(fd);
		}
	}
}


static void usage(void)
{
	printf("-p port\n-t socket options\n-b bufsize\n\n");
}

int main(int argc, char *argv[])
{
	int opt;
	extern char *optarg;

	while ((opt = getopt (argc, argv, "p:t:H:b:h")) != EOF) {
		switch (opt) {
		case 'p':
			port = atoi(optarg);
			break;
		case 't':
			tcp_options = optarg;
			break;
		case 'b':
			bufsize = atoi(optarg);
			break;

		case 'h':
		default:
			usage();
			exit(1);
		}
	}

	printf("port=%d options=[%s]\n", port, tcp_options);

	listener();

	return 0;
}
