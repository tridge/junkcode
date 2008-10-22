/*
  force TCP connections to come from a specific IP

  compile with:
     gcc -o connectfrom.so -fPIC -shared -o connectfrom.so connectfrom.c -ldl
*/

#include <stdio.h>
#include <pwd.h>
#include <errno.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen)
{
	static int (*real_connect)(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);
	const char *address = getenv("CONNECTFROM");
	
	if (!real_connect) {
		real_connect = dlsym((void*)-1, "connect");
	}

	if (address) {
		struct sockaddr_in myaddr;
		socklen_t myaddrlen = sizeof(myaddr);
		myaddr = *(struct sockaddr_in *)serv_addr;
		inet_aton(address, &myaddr.sin_addr);
		myaddr.sin_port = 0;
		
		bind(sockfd, (struct sockaddr *)&myaddr, myaddrlen);
	}

	return real_connect(sockfd, serv_addr, addrlen);
}
