#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

typedef unsigned uint32;

struct packet {
	uint32 tag;
	uint32 length;
	uint32 offset;
	char buf[8192];
};

#define PROXY_PORT 0x5641 /* VA in hex */
#define PROXY_TAG 0x42
#define WEB_PORT 80

#ifndef INADDR_BCAST
#define INADDR_BCAST 0xffffffff
#endif

int open_socket_in(int type, const char *dst, int port);
int open_socket_out(int type, const char *dst, int port);
void write_all(int fd, void *buf, size_t size);
