#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


main(int argc, char *argv[])
{
	char *name = argv[1];
	struct hostent *hp;
	struct in_addr addr;

	if (inet_aton(name, &addr) != 0) {
		printf("It's an IP\n");
	} else {
		hp = gethostbyname(name);
	}
}

