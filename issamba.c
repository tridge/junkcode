#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


static int open_socket_out(char *host, int port)
{
	struct sockaddr_in sock_out;
	int res;
	struct hostent *hp;

	res = socket(PF_INET, SOCK_STREAM, 0);

	hp = gethostbyname(host);
	if (!hp) {
		fprintf(stderr,"unknown host: %s\n", host);
		exit(1);
	}

	memcpy(&sock_out.sin_addr, hp->h_addr, hp->h_length);
	sock_out.sin_port = htons(port);
	sock_out.sin_family = PF_INET;

	if (connect(res,(struct sockaddr *)&sock_out,sizeof(sock_out))) {
		fprintf(stderr, "failed to connect to %s - %s\n", host, strerror(errno));
		exit(1);
	}

	return res;
}

int main(int argc, char *argv[])
{
	int port = 139;
	char *host;
	unsigned char buf[4];
	int sock;

	if (argc < 2) {
		printf("usage: issamba <host>\n");
		exit(1);
	}

	host = argv[1];
	bzero(buf, 4);
	buf[0] = 0x89;

	sock = open_socket_out(host, port);

	if (write(sock, buf, 4) != 4) {
		fprintf(stderr,"Failed to send request\n");
		exit(1);
	}

	bzero(buf, 4);
	if (read(sock, buf, 4) != 4) {
		fprintf(stderr,"Failed to recv request\n");
		exit(1);
	}

	if (buf[0] == 0x85) {
		printf("%s is a Samba server\n", host);
	} else {
		printf("%s is not a Samba server (0x%02x)\n", host, buf[0]);
	}
	return 0;
}
