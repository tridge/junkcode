#define _GNU_SOURCE

#include <stdio.h>
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

	inet_aton(host, &addr);

	sock_out.sin_addr = addr;
	sock_out.sin_port = htons(port);
	sock_out.sin_family = PF_INET;

	if (connect(res,(struct sockaddr *)&sock_out,sizeof(sock_out)) != 0) {
		close(res);
		fprintf(stderr,"failed to connect to %s (%s)\n", 
			host, strerror(errno));
		return -1;
	}

	return res;
}


static void unix_time(time_t t)
{
	struct tm *tm;

	tm = gmtime(&t);
	
	printf("%04d%02d%02d%02d%02d%02d\n",
	       tm->tm_year + 1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, 
	       tm->tm_min, tm->tm_sec);
}

int main(int argc, char *argv[])
{
	int sock;
	unsigned t, ofs;
	char *host;

	if (argc != 3) {
		printf("rtime <server> <offset>\n");
		exit(1);
	}

	host = argv[1];
	ofs = atoi(argv[2]);

	sock = open_socket_out(argv[1], 37);

	if (read(sock, &t, sizeof(t)) == 4) {
		unix_time(ofs + ntohl(t) - 2208988800U);
		return 0;
	}
	return -1;
}
