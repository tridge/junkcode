#include <sys/epoll.h>
#include <errno.h>

/*
 written by Stefan Metzmacher <metze@samba.org>

 this code calls epoll_wait() in a endless loop with a
 timeout of 1000 = 1s, before the loop we add one event
 to epoll via epoll_ctl(EPOLL_CTL_ADD), we add it with an event mask
 of 0, so we don't request notification for any events!

 this test demonstrates that we'll get an EPOLLHUP notification,
 when the fd was closed by the other end of the pipe/socket

 this programm used the stdin (fd=0).
 
 so use it like this:
 (sleep 5 &) | ./epoll_test
*/
int main(void) {
	int epoll_fd;
	struct epoll_event event;
	int socket_fd;
	int ret, i;
#define MAXEVENTS 1
	struct epoll_event events[MAXEVENTS];
	int timeout = -1;

	epoll_fd = epoll_create(64);

	socket_fd = 0;

	memset(&event, 0, sizeof(event));
	event.events = 0;
	event.data.fd = socket_fd;
	ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &event);
	printf("epoll_ctl:ADD: %d\n", ret);

	while (1) {
		ret = epoll_wait(epoll_fd, events, MAXEVENTS, 1000);
		if (ret == -1 && errno != EINTR) {
			return -1;
		}
		printf("epool_wait: ret[%d] errno[%d]\n",ret, errno);
	
		for (i=0;i<ret;i++) {
			printf("event i[%d] fd[%d] events[0x%08X]", i, events[i].data.fd, events[i].events);
			if (events[i].events & EPOLLIN) printf(" EPOLLIN");
			if (events[i].events & EPOLLHUP) printf(" EPOLLHUP");
			if (events[i].events & EPOLLERR) printf(" EPOLLERR");
			if (events[i].events & EPOLLOUT) printf(" EPOLLOUT");
			printf("\n");
		}
	}

	return 0;
}
