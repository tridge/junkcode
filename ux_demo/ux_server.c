/* simple unix domain socket server demo 
   tridge@samba.org, January 2002
*/

/*
  this server will listen for connections on a unix domain socket and will additionally
  wait for events on the snap8051 device
*/

#include "ux_demo.h"

/* this is how often we do regular timer processing when idle */
#define SLEEP_TIME 5000

/* each client connected on the unix domain socket gets one of these */
struct client {
	int fd;
	struct client *next, *prev;
};

/* a linked list of current clients */
static struct client *clients;
static unsigned num_clients;

/* 
   this is called every SLEEP_TIME milliseconds when idle 
*/
static void timer_processing(void)
{
	printf("tick!\n");
}

#if SNAP
/*
  handle a k8051 event
*/
static void k8051_event(int fd)
{
	struct k8051_event event;

	event.event_mask = K8051_EVENT_ALL;

	if (ioctl(fd, IOCTL_SNAP_NOBLOCK_EVENT, &event) != 0) return;

	printf("Got 8051 event 0x%02x\n", event.reply_pkt[6]);
}
#endif

/*
  handle a new client joining
*/
static void new_client(int ux_sock)
{
	struct client *c;
	struct sockaddr addr;
	socklen_t len = sizeof(addr);
	int fd;

	fd = accept(ux_sock, &addr, &len);
	if (fd == -1) return;

	/* put it in our linked list */
	c = (struct client *)x_malloc(sizeof(*c));
	memset(c, 0, sizeof(*c));
	c->fd = fd;
	c->next = clients;
	clients = c;

	num_clients++;

	printf("Another sheep has joined\n");
}

/*
  kill off a dead client
*/
static void dead_client(struct client *c)
{
	close(c->fd);
	if (c->prev) c->prev->next = c->next;
	if (c->next) c->next->prev = c->prev;
	if (c == clients) clients = c->next;
	free(c);
	num_clients--;
	printf("We've lost a sheep\n");
}

/*
  process a single request
*/
static void process_request(struct client *c, const char *req_buf, size_t len)
{
	char *reply;

	printf("Got request [%*.*s]\n", len, len, req_buf);
	asprintf(&reply, "You said '%*.*s'", len, len, req_buf);

	if (send_packet(c->fd, reply, strlen(reply)) != 0) {
		dead_client(c);
	}

	free(reply);
}

/*
  a client wants to send us something, or has exited
*/
static void client_input(struct client *c)
{
	size_t len;
	char *inbuf;

	if (recv_packet(c->fd, &inbuf, &len) != 0) {
		dead_client(c);
		return;
	}

	process_request(c, inbuf, len);
	free(inbuf);
}

/* main processing loop */
static void process(int ux_sock, int snap8051_fd)
{
	struct pollfd *polls = NULL;

	while (1) {
		struct client *c;
		int i, poll_count;

		/* setup for a poll */
		polls = x_realloc(polls, (2+num_clients) * sizeof(*polls));
		polls[0].fd = ux_sock;
		polls[0].events = POLLIN;

#if SNAP
		/* notice the strange poll mask for the 8051 */
		polls[1].fd = snap8051_fd;
		polls[1].events = K8051_EVENT_ALL;
#endif

		/* setup the clients */
		for (i=2, c = clients; c; i++, c = c->next) {
			polls[i].fd = c->fd;
			polls[i].events = POLLIN;
		}

		/* most of our life is spent in this call */
		poll_count = poll(polls, i, SLEEP_TIME);
		
		if (poll_count == -1) {
			/* something went badly wrong! */
			perror("poll");
			exit(1);
		}

		if (poll_count == 0) {
			/* timeout */
			timer_processing();
			continue;
		}

		/* see if a client wants to speak to us */
		for (i=2, c = clients; c; i++) {
			struct client *next = c->next;
			if (polls[i].revents & POLLIN) {
				client_input(c);
			}
			c = next;
		}

		/* see if we got a new client */
		if (polls[0].revents & POLLIN) {
			new_client(ux_sock);
		}

#if SNAP
		/* see if we got a 8051 event */
		if (polls[1].revents) {
			k8051_event(snap8051_fd);
		}
#endif
	}
}


/* main program */
int main(int argc, char *argv[])
{
	int snap8051_fd = -1;
	int ux_sock;

	ux_sock = ux_socket_listen(SOCKET_NAME);
	if (ux_sock == -1) {
		perror("ux_socket_listen");
		exit(1);
	}

	snap8051_fd = open("/dev/snap8051", O_RDONLY);
	if (snap8051_fd == -1) {
		perror("/dev/snap8051");
		exit(1);
	}

	process(ux_sock, snap8051_fd);
	
	return 0;
}
