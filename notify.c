#define DN_ACCESS       0x00000001      /* File accessed in directory */
#define DN_MODIFY       0x00000002      /* File modified in directory */
#define DN_CREATE       0x00000004      /* File created in directory */
#define DN_DELETE       0x00000008      /* File removed from directory */
#define DN_RENAME       0x00000010      /* File renamed in directory */
#define DN_ATTRIB       0x00000020      /* File changed attribute */
#define DN_MULTISHOT    0x80000000      /* Don't remove notifier */
#define F_NOTIFY 1026

#define _GNU_SOURCE	/* needed to get the defines */
#include <fcntl.h>	/* in glibc 2.2 this has the needed
				   values defined */
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

static int event_fd;

static void handler(int sig, siginfo_t *si, void *data)
{
	event_fd = si->si_fd;
}

int main(void)
{
	struct sigaction act;
	int fd;
	
	act.sa_sigaction = handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	sigaction(SIGRTMIN, &act, NULL);
	
	fd = open(".", O_RDONLY);
	fcntl(fd, F_SETSIG, SIGRTMIN);
	fcntl(fd, F_NOTIFY, DN_MODIFY|DN_CREATE|DN_MULTISHOT);
	/* we will now be notified if any of the files
	   in "dir" is modified or new files are
	   created */
	while (1) {
		pause();
		printf("Got event on fd=%d\n", event_fd);
	}
}
