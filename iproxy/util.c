#include "iproxy.h"

/****************************************************************************
open a socket of the specified type, port and address for incoming data
****************************************************************************/
int open_socket_in(int type, const char *dst, int port)
{
	struct sockaddr_in sock;
	int res;
	int one = 1;
	struct in_addr addr;

	if (inet_pton(AF_INET, dst, &addr) <= 0) {
		perror(dst);
	}

	memset((char *)&sock,'\0',sizeof(sock));

#ifdef HAVE_SOCK_SIN_LEN
	sock.sin_len = sizeof(sock);
#endif
	sock.sin_port = htons(port);
	sock.sin_family = AF_INET;
	sock.sin_addr = addr;
	res = socket(AF_INET, type, 0);
	if (res == -1) {
		perror("socket");
		return -1;
	}

	setsockopt(res,SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof(one));

	if (bind(res, (struct sockaddr * )&sock,sizeof(sock)) == -1) {
		perror("bind");
		return -1;
	}

	return res;
}


/****************************************************************************
  create an outgoing socket. 
  **************************************************************************/
int open_socket_out(int type, const char *dst, int port)
{
	struct sockaddr_in sock_out;
	int res;
	int one=1;
	struct in_addr addr;

	if (inet_pton(AF_INET, dst, &addr) <= 0) {
		perror(dst);
	}

	res = socket(PF_INET, type, 0);
	if (res == -1) {
		perror("socket");
		return -1;
	}

	memset((char *)&sock_out,'\0',sizeof(sock_out));
	sock_out.sin_addr = addr;
	sock_out.sin_port = htons(port);
	sock_out.sin_family = AF_INET;

	setsockopt(res,SOL_SOCKET,SO_BROADCAST,(char *)&one,sizeof(one));

	if (connect(res,(struct sockaddr *)&sock_out,sizeof(sock_out)) != 0) {
		perror("connect");
		return -1;
	}

	return res;
}

void write_all(int fd, void *buf, size_t size)
{
	while (size) {
		size_t n = write(fd, buf, size);
		if (n <= 0) {
			perror("write");
			exit(1);
		}
		size -= n;
		buf += n;
	}
}
