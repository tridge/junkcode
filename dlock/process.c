/* a simple distributed lock manager server

   Andrew Tridgell (tridge@samba.org), February 2003

*/

#include "dl_server.h"
#include "dlinklist.h"


/* current locks are kept in a linked list. Each element in the linked
   list has another linked list of pending locks hanging off it */
struct lock_list {
	struct lock_list *next, *prev;
	struct lock_list *pending;

	/* the sock_fd and sin specify how to send a reply to the client
	   when the lock is eventually granted */
	int sock_fd;
	struct sockaddr_in sin;

	/* 32 bit lock ranges will do for this test */
	uint32 offset;
	uint32 size;
};

static struct lock_list *lock_list;


#define SWAP_IN_32(x) ((x) = ntohl(x))
#define SWAP_OUT_32(x) ((x) = htonl(x))

/*
  byteswap the request to native byte order - a poor mans linearizer
*/
static void swap_request(struct dl_request *request)
{
	SWAP_IN_32(request->offset);
	SWAP_IN_32(request->size);
}

/*
  byteswap the reply to native byte order
*/
static void swap_reply(struct dl_reply *reply)
{
	SWAP_OUT_32(reply->status);
}


/*
  process an lock request with a lock_list element
  if there is a conflict then add the lock to the pending list
  for the conflicting lock. 
*/
static void do_lock(struct lock_list *lock)
{
	struct lock_list *list;
	struct dl_reply reply;
	int fromlen;

	for (list=lock_list;list;list=list->next) {
		if (lock->offset+lock->size > list->offset &&
		    lock->offset < list->offset+list->size) {
#if DEBUG
			printf("pending lock at %d from %d\n", 
			       lock->offset, lock->sin.sin_port);
#endif
			/* conflict - make it pending */
			DLIST_ADD(list->pending, lock);
			return;
		}
	}

#if DEBUG
	printf("granted lock at %d to %d\n", 
	       lock->offset, lock->sin.sin_port);
#endif

	/* no conflict - add it to the list of current locks */
	DLIST_ADD(lock_list, lock);

	reply.status = NT_STATUS_OK;
	swap_reply(&reply);

	/* and send the reply */
	fromlen = sizeof(lock->sin);
	sendto(lock->sock_fd, &reply, sizeof(reply), 0, 
	       (struct sockaddr *)&lock->sin, fromlen);	
}

/*
  process an unlock request
*/
static void process_unlock(uint32 offset, uint32 size,
			   int sock_fd, struct sockaddr_in *from)
{
	struct lock_list *list, *pending;
	struct dl_reply reply;
	int fromlen;

	for (list=lock_list;list;list=list->next) {
		if (offset == list->offset &&
		    size == list->size &&
		    from->sin_addr.s_addr == list->sin.sin_addr.s_addr &&
		    from->sin_port == list->sin.sin_port) {
#if DEBUG
			printf("removed lock at %d from %d\n", 
			       offset, list->sin.sin_port);
#endif
			DLIST_REMOVE(lock_list, list);
			break;
		}
	}

	if (list) {
		reply.status = NT_STATUS_OK;
	} else {
		reply.status = NT_STATUS_RANGE_NOT_LOCKED;
	}

	swap_reply(&reply);
	
	/* and send the reply */
	fromlen = sizeof(*from);
	sendto(sock_fd, &reply, sizeof(reply), 0, (struct sockaddr *)from, fromlen);

	if (!list) {
		return;
	}

	pending = list->pending;
	free(list);

	/* process any pending locks */
	while (pending) {
		list = pending;
		DLIST_REMOVE(pending, list);
		do_lock(list);
	}
}


/*
  process a lock request
*/
static void process_lock(uint32 offset, uint32 size,
			 int sock_fd, struct sockaddr_in *from)
{
	struct lock_list *list;

	list = malloc(sizeof(*list));

	list->pending = NULL;
	list->sock_fd = sock_fd;
	list->sin = *from;
	list->offset = offset;
	list->size = size;

	do_lock(list);
}

/*
  this implements the simple protocol logic
*/
void process_packet(char *buf, size_t length, int sock_fd, struct sockaddr_in *from)
{
	struct dl_request request;

	if (length != sizeof(request)) {
		fatal("invalid request size %u - expected %u\n",
		      length, sizeof(request));
	}

	/* parse the incoming packet */
	memcpy(&request, buf, sizeof(request));
	swap_request(&request);

	switch (request.lock_type) {
	case WRITE_LOCK:
		process_lock(request.offset, request.size, sock_fd, from);
		break;

	case UN_LOCK:
		process_unlock(request.offset, request.size, sock_fd, from);
		break;
	}
}
