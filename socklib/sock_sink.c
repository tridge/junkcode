#include "socklib.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



static char *tcp_options="TCP_NODELAY IPTOS_THROUGHPUT";
static int port=7001;
static int bufsize=0x10000;
static char *host="127.0.0.1";
static const char *outfile;
static unsigned delay_per_packet;

static void sender(void)
{
	int fd, fd2 = -1;
	uint64_t total=0;
	char *buf;
	
	fd = open_socket_out(host, port);

	if (fd == -1) exit(1);

	set_socket_options(fd, tcp_options);

	buf = (char *)malloc(bufsize);

	if (!buf) {
		fprintf(stderr,"out of memory\n");
		exit(1);
	}

	memset(buf, 'Z', bufsize);

	start_timer();

	if (outfile) {
		fd2 = open(outfile, O_WRONLY|O_CREAT|O_TRUNC, 0600);
	}

	while (1) {
		int ret = read(fd, buf, bufsize);
		if (ret <= 0) break;
		if (fd2 != -1) {
			if (write(fd2, buf, ret) != ret) {
				printf("Write error to %s\n", outfile);
				exit(1);
			}
		}
		if (delay_per_packet) {
			usleep(delay_per_packet);
		}
		total += ret;
		if (end_timer() > 2.0) {
			report_time(total);
			total = 0;
			start_timer();
		}
	}
}


static void usage(void)
{
	printf("-p port\n-t socket options\n-H host\n-b bufsize [-F outfile] [-D delay]\n\n");
}

int main(int argc, char *argv[])
{
	int opt;
	extern char *optarg;

	while ((opt = getopt (argc, argv, "p:t:H:b:hF:D:")) != EOF) {
		switch (opt) {
		case 'p':
			port = strtol(optarg, NULL, 0);
			break;
		case 't':
			tcp_options = optarg;
			break;
		case 'H':
			host = optarg;
			break;
		case 'b':
			bufsize = strtol(optarg, NULL, 0);
			break;
		case 'F':
			outfile = optarg;
			break;
		case 'D':
			delay_per_packet = strtoul(optarg, NULL, 0);
			break;

		case 'h':
		default:
			usage();
			exit(1);
		}
	}

	printf("host=%s port=%d bufsize=%d options=[%s]\n", host, port, bufsize, tcp_options);

	sender();

	return 0;
}
