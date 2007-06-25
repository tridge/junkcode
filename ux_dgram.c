#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>


/*
  connect to a unix domain socket
*/
int ux_socket_connect(const char *name)
{
	int fd;
        struct sockaddr_un addr;

        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, name, sizeof(addr.sun_path));

	fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (fd == -1) {
		return -1;
	}
	
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		close(fd);
		return -1;
	}

	return fd;
}


/*
  create a unix domain socket and bind it
  return a file descriptor open on the socket 
*/
static int ux_socket_bind(const char *name)
{
	int fd;
        struct sockaddr_un addr;

	/* get rid of any old socket */
	unlink(name);

	fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (fd == -1) return -1;

        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, name, sizeof(addr.sun_path));

        if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		close(fd);
		return -1;
	}	

	return fd;
}


void *x_malloc(size_t size)
{
	void *ret;
	ret = malloc(size);
	if (!ret) {
		fprintf(stderr,"Out of memory for size %d\n", (int)size);
		exit(1);
	}
	return ret;
}

void *x_realloc(void *ptr, size_t size)
{
	void *ret;
	ret = realloc(ptr, size);
	if (!ret) {
		fprintf(stderr,"Out of memory for size %d\n", (int)size);
		exit(1);
	}
	return ret;
}

/*
  keep writing until its all sent
*/
int write_all(int fd, const void *buf, size_t len)
{
	size_t total = 0;
	while (len) {
		int n = write(fd, buf, len);
		if (n <= 0) return total;
		buf = n + (char *)buf;
		len -= n;
		total += n;
	}
	return total;
}



int main(void)
{
	const char *fname = "ux_dgram.dat";
	int fd, fd2, r;
	ssize_t s;
	char buf[1024];
	struct sockaddr_un from;
	socklen_t fromlen=sizeof(from);

	fd = ux_socket_bind(fname);
	
	fd2 = ux_socket_connect(fname);

	dprintf(fd2, "Hello world\n");

	memset(&from, 0, sizeof(from));

	s = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *)&from, &fromlen);
	printf("Received %d bytes from '%s' family=%d\n", s, 
	       from.sun_path, from.sun_family);

	r = connect(fd, (struct sockaddr *)&from, sizeof(from));
	printf("connect gave %d\n", r);
	
	s = sendto(fd, buf, s, 0, (struct sockaddr *)&from, sizeof(from));
	printf("Sent %d bytes\n", s);

	return 0;
}
