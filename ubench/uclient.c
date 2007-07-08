#include "ubench.h"

static int port=7001;
#define BUFSIZE 0x10000
static int recv_size = 0x2000;
static int send_size = 0x2000;
static int backlog = 2;

static void set_nonblocking(int fd)
{
	unsigned v = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, v | O_NONBLOCK);
}

static struct timeval tp1,tp2;

void start_timer()
{
	gettimeofday(&tp1,NULL);
}

double end_timer()
{
	gettimeofday(&tp2,NULL);
	return((tp2.tv_sec - tp1.tv_sec) + 
	       (tp2.tv_usec - tp1.tv_usec)*1.0e-6);
}

void report_time(int total)
{
	printf("%.6g MB/sec\n", (total/(1.0e6))/end_timer());
}

static void load_gen(int fd)
{
	int *buf;
	unsigned pkts=0;
	unsigned i=0;
	unsigned max_recvd=0;

	buf = malloc(BUFSIZE);

	if (backlog) {
		set_nonblocking(fd);
	}

	start_timer();

	while (1) {
		int n;
		buf[0] = htonl(recv_size);
		buf[1] = i++;
		write(fd, buf, send_size);
		n = read(fd, buf, BUFSIZE);
		if (n == recv_size) {
			pkts++;
			if (buf[1] > max_recvd) max_recvd = buf[1];
			if (max_recvd > i) max_recvd = i;
		}
		if (backlog && i - max_recvd >= backlog) {
			fd_set fds;
			struct timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = 10000;
			FD_ZERO(&fds);
			FD_SET(fd, &fds);
			select(fd+1, &fds, NULL, NULL, &tv);
		}
		if (end_timer() > 1.0) {
			report_time(pkts*(recv_size+send_size));
			start_timer();
			pkts=0;
		}
	}
}

static void client(char *host)
{
	struct sockaddr_in sock;
	int fd;
	int one=1;
	struct sockaddr_in sock_out;
	struct hostent *hp;  

	bzero((char *)&sock,sizeof(sock));
	sock.sin_port = 0;
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

	hp = gethostbyname(host);
	if (!hp) {
		fprintf(stderr,"unknown host: %s\n", host);
		return;
	}

	memcpy(&sock_out.sin_addr, hp->h_addr, hp->h_length);
	sock_out.sin_port = htons(port);
	sock_out.sin_family = PF_INET;

	if (connect(fd,(struct sockaddr *)&sock_out,sizeof(sock_out))) {
		close(fd);
		fprintf(stderr,"failed to connect to %s - %s\n", 
			host, strerror(errno));
		return;
	}

	load_gen(fd);
}


static void usage(void)
{
	printf("\n"
"uclient <options>\n"
"  -H host\n"
"  -p port\n"
"  -b backlog\n"
"  -r recv_size\n"
"  -s send_size\n");
}

int main(int argc, char *argv[])
{
	int opt;
	extern char *optarg;
	char *host = "localhost";

	while ((opt = getopt (argc, argv, "hH:r:s:p:b:")) != EOF) {
		switch (opt) {
		case 'H':
			host = optarg;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'b':
			backlog = atoi(optarg);
			break;
		case 'r':
			recv_size = atoi(optarg);
			break;
		case 's':
			send_size = atoi(optarg);
			break;
		case 'h':
		default:
			usage();
			exit(1);
		}
	}

	printf("recv_size=%d send_size=%d host=%s:%d\n", recv_size, send_size, 
	       host, port);
	client(host);

	return 0;
}
