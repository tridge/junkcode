#define _GNU_SOURCE
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX(a,b) ((a)>(b)?(a):(b))

/* open a socket to a tcp remote host with the specified port */
static int open_socket_out(const char *host, int port)
{
	struct sockaddr_in sock_out;
	int res;
	struct hostent *hp;  
	struct in_addr addr;

	res = socket(PF_INET, SOCK_STREAM, 0);
	if (res == -1) {
		return -1;
	}

	if (inet_pton(AF_INET, host, &addr) > 0) {
		memcpy(&sock_out.sin_addr, &addr, sizeof(addr));
	} else {
		hp = gethostbyname(host);
		if (!hp) {
			fprintf(stderr,"tseal: unknown host %s\n", host);
			return -1;
		}
		memcpy(&sock_out.sin_addr, hp->h_addr, hp->h_length);
	}

	sock_out.sin_port = htons(port);
	sock_out.sin_family = PF_INET;

	if (connect(res,(struct sockaddr *)&sock_out,sizeof(sock_out)) != 0) {
		close(res);
		fprintf(stderr,"tseal: failed to connect to %s (%s)\n", 
			host, strerror(errno));
		return -1;
	}

	return res;
}

/* write to a file descriptor, making sure we get all the data out or
 * die trying */
static void write_all(int fd, unsigned char *s, size_t n)
{
	while (n) {
		int r;
		r = write(fd, s, n);
		if (r <= 0) {
			exit(1);
		}
		s += r;
		n -= r;
	}
}


static int new_file(time_t *t)
{
	int fd;

	*t = time(NULL);

	errno = 0;

	do {
		char *name;
		asprintf(&name, "iol-%u.tmp", (unsigned)*t);
		fd = open(name, O_CREAT|O_EXCL|O_WRONLY, 0644);
		if (fd == -1) {
			(*t)++;
		}
		free(name);
	} while (fd == -1 && *t < time(NULL) + 10000);

	return fd;
}

static void end_file(time_t t)
{
	char *name1, *name2;

	asprintf(&name1, "iol-%u.tmp", (unsigned)t);	
	asprintf(&name2, "iol-%u.dat", (unsigned)t);

	if (rename(name1, name2) != 0) {
		perror(name2);
	}

	free(name1);
	free(name2);
}

static int check_checksum(const char *line, size_t len)
{
	unsigned char sum = 0;
	int i;

	for (i=0;i<len-3;i++) {
		sum += ((unsigned char *)line)[i];
	}

	if ((unsigned char)(line[len-3]) == sum) {
		return 1;
	}

	printf("Bad checksum 0x%2x - expected 0x%2x\n",
	       sum, (unsigned)line[len-3]);
	return 0;
}

static int fetch_line(int fd, char **line)
{
	int len = 0;
	size_t alloc_len = 2000;

	*line = malloc(alloc_len);
	if (! *line) {
		return -1;
	}

	while (1) {
		char c;

		if (len == alloc_len) {
			alloc_len *= 2;
			*line = realloc(*line, alloc_len);
			if (! *line) {
				return -1;
			}
		}

		if (read(fd, &c, 1) != 1) {
			return len;
		}

		(*line)[len++] = c;
		if (len > 3 && (*line)[len-2] == '\r' && (*line)[len-1] == '\n') {
			if (len == 4) {
				return len;
			}

			if (check_checksum(*line, len)) {
				return len;
			}
		}
	}

	return -1;
}

static void main_loop(int fd)
{
	int len;
	int data_fd = -1;
	time_t t;

	while (1) {
		char *line = NULL;

		len = fetch_line(fd, &line);
		if (len < 4 || !line) {
			printf("io error - got len %d\n", len);
			exit(1);
		}

		if (strncmp(line, "ED\r\n", len) == 0) {
			if (data_fd != -1) {
				close(data_fd);
				data_fd = -1;
			}
			data_fd = new_file(&t);
			if (data_fd == -1) {
				perror("newfile");
				exit(1);
			}
		}

		if (data_fd == -1) {
			printf("Expected ED first\n");
			exit(1);
		}

		write_all(data_fd, line, len);

		if (strncmp(line, "EE\r\n", len) == 0) {
			close(data_fd);
			data_fd = -1;
			end_file(t);
		}

		if (dprintf(fd, "%c%c\r\n", tolower(line[0]), tolower(line[1])) != 4) {
			printf("Failed to ack packet\n");
			exit(1);
		}

		free(line);
	}
}

int main(int argc, char *argv[])
{
	int dest_port;
	char *host;
	int sock_out;

	if (argc < 3) {
		printf("Usage: iolmaster  <host> <port>\n");
		exit(1);
	}

	host = argv[1];
	dest_port = atoi(argv[2]);

	sock_out = open_socket_out(host, dest_port);
	if (sock_out == -1) {
		exit(1);
	}

	main_loop(sock_out);
	return 0;
}
