#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

int main(int argc, char *argv[])
{
	struct sockaddr sa;
	int length = sizeof(sa);
	
	getpeername(0, &sa, &length);

	connect(0, &sa, length);

	return execvp(argv[1], argv+1);
}
