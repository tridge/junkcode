/*
  test program to demonstrate use of signals with setreuid() and setresuid()

  tridge@samba.org August 2008
 */

#define _XOPEN_SOURCE 500
#define _GNU_SOURCE 

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <utime.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <aio.h>


static void sig_handler(int signum)
{
	printf("Got signal %d\n", signum);
}

int main(int argc, const char *argv[])
{
	signal(SIGUSR1, sig_handler);
	signal(SIGUSR2, sig_handler);
	alarm(5);

	if (fork() == 0) {
		setreuid(-1, 1);
		sleep(1);
		kill(getppid(), SIGUSR1);
		sleep(1);
		setreuid(-1, 0);
		setresuid(1, 1, -1);
		kill(getppid(), SIGUSR2);
	}

	pause();
	pause();

	return 0;
}
