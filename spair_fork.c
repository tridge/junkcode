#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>

static int dispatch_daemon(int fd)
{
	int epoll_fd = epoll_create(64);
	struct epoll_event event;
	int v;

	while (read(fd, &v, sizeof(v)) == 

}


int main(void) 
{ 
       int fd[2];
       int i;
       const int n = 10;

       setpgrp();

       if (socketpair(AF_UNIX, SOCK_DGRAM, 0, fd) != 0) {
	       perror("socketpair");
	       exit(1);
       }

       if (fork() == 0) {
	       close(fd[0]);
	       dispatch_daemon(fd[1]);
       }

       for (i=0;i<n;i++) {
	       if (fork() == 0) {
		       close(fd[1]);
		       client(fd[0]);
	       }
       }
       close(fd[0]);
       close(fd[1]);

       sleep(10);
       
       kill(-getpid(), SIGTERM);
       return 0;
} 
