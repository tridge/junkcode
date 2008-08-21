#include "socklib.h"


static char *tcp_options="TCP_NODELAY IPTOS_THROUGHPUT";
static int port=7001;
static int bufsize=0x10000;
static int use_sendfile;

#if WITH_SENDFILE
#include <sys/sendfile.h>

static int do_write(int fd, const char *buf, size_t size)
{
	off_t offset = 0;
	static int tmp_fd = -1;
	char template[] = "/tmp/socklib.XXXXXX";

	if (!use_sendfile) {
		return write(fd, buf, size);
	}

	if (tmp_fd == -1) {
		tmp_fd = mkstemp(template);
		if (tmp_fd == -1) {
			perror("mkstemp");
			exit(1);
		}
		unlink(template);
		write(tmp_fd, buf, size);
	}

	return sendfile(fd, tmp_fd, &offset, size);
}
#else
static int do_write(int fd, const char *buf, size_t size)
{
	return write(fd, buf, size);
}
#endif

static void server(int fd)
{
	char *buf;
	uint64_t total=0;

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
		int ret = do_write(fd, buf, bufsize);
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
	printf("Use -S to enable sendfile\n");
}

int main(int argc, char *argv[])
{
	int opt;
	extern char *optarg;

	while ((opt = getopt (argc, argv, "p:t:H:b:hS")) != EOF) {
		switch (opt) {
		case 'p':
			port = strtol(optarg, NULL, 0);
			break;
		case 'S':
#if WITH_SENDFILE
			use_sendfile = 1;
#else
			printf("sendfile not compiled in\n");
			exit(1);
#endif
			break;
		case 't':
			tcp_options = optarg;
			break;
		case 'b':
			bufsize = strtol(optarg, NULL, 0);
			break;

		case 'h':
		default:
			usage();
			exit(1);
		}
	}

	printf("port=%d bufsize=%d options=[%s]\n", port, bufsize, tcp_options);

	listener();

	return 0;
}
