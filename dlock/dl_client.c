/* a simple distributed lock manager client

   Andrew Tridgell (tridge@samba.org), February 2003

*/

#include "dl_server.h"

#define SWAP_IN_32(x) ((x) = ntohl(x))
#define SWAP_OUT_32(x) ((x) = htonl(x))

/*
  byteswap the request to native byte order - a poor mans linearizer
*/
static void swap_out_request(struct dl_request *request)
{
	SWAP_OUT_32(request->offset);
	SWAP_OUT_32(request->size);
}

/*
  byteswap the reply to native byte order
*/
static void swap_in_reply(struct dl_reply *reply)
{
	SWAP_IN_32(reply->status);
}


uint32 lock_range(struct dl_handle *handle, int offset, int len)
{
	struct dl_request request;
	struct dl_reply reply;

	request.offset = offset;
	request.size = len;
	
	request.lock_type = WRITE_LOCK;

	swap_out_request(&request);

	write_all(handle->fd, &request, sizeof(request));
	read_all(handle->fd, &reply, sizeof(reply));

	swap_in_reply(&reply);

	if (reply.status) {
		printf("lock at %d -> %d\n", offset, reply.status);
	}
	return reply.status;
}


uint32 unlock_range(struct dl_handle *handle, int offset, int len)
{
	struct dl_request request;
	struct dl_reply reply;

	request.offset = offset;
	request.size = len;
	
	request.lock_type = UN_LOCK;

	swap_out_request(&request);

	write_all(handle->fd, &request, sizeof(request));
	read_all(handle->fd, &reply, sizeof(reply));

	swap_in_reply(&reply);
	
	if (reply.status) {
		printf("unlock at %d -> %d\n", offset, reply.status);
	}

	return reply.status;
}


struct dl_handle *dlock_open(const char *host, int port)
{
	struct sockaddr_in sock;
	int fd;
	int one=1;
	struct sockaddr_in sock_out;
	struct hostent *hp;  
	struct dl_handle *handle;

	bzero((char *)&sock,sizeof(sock));
	sock.sin_port = 0;
	sock.sin_family = AF_INET;
	sock.sin_addr.s_addr = 0;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1) { 
		fatal("socket failed - %s\n", strerror(errno));
	}

	setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof(one));

	/* now we've got a socket - we need to bind it */
	if (bind(fd, (struct sockaddr * ) &sock,sizeof(sock)) < 0) { 
		fatal("bind error - %s\n", strerror(errno));
	}

	hp = gethostbyname(host);
	if (!hp) {
		fatal("unknown host: %s\n", host);
	}

	memcpy(&sock_out.sin_addr, hp->h_addr, hp->h_length);
	sock_out.sin_port = htons(port);
	sock_out.sin_family = PF_INET;

	if (connect(fd,(struct sockaddr *)&sock_out,sizeof(sock_out))) {
		close(fd);
		fatal("failed to connect to %s - %s\n", 
		      host, strerror(errno));
	}

	handle = malloc(sizeof(*handle));
	handle->fd = fd;
	return handle;
}
