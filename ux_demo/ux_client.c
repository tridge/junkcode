/* simple unix domain socket client demo 
   tridge@samba.org, January 2002
*/

#include "ux_demo.h"


/*
  process the client 
*/
static void process(int fd)
{
	char line[1000];
	char *reply;

	printf("Type what you want to send\n");

	while (fgets(line, sizeof(line), stdin)) {
		unsigned len = strlen(line);

		if (line[len-1] == '\n') {
			line[len-1] = 0;
			len--;
		}
		
		if (send_packet(fd, line, strlen(line)) != 0) break;
		if (recv_packet(fd, &reply, &len) != 0) break;

		printf("Got reply [%*.*s]\n",
		       len, len, reply);
		free(reply);
	}
}

/* main program */
int main(int argc, char *argv[])
{
	int fd;

	fd = ux_socket_connect(SOCKET_NAME);
	if (fd == -1) {
		perror("ux_socket_connect");
		exit(1);
	}

	process(fd);
	
	return 0;
}
