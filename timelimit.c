/* run a command with a limited timeout
   tridge@samba.org, June 2005

   attempt to be as portable as possible (fighting posix all the way)
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

static void usage(void)
{
	printf("usage: timelimit <time> <command>\n");
}

static void sig_alrm(int sig)
{
	fprintf(stderr, "\nMaximum time expired in timelimit - killing\n");
	kill(0, SIGKILL);
	exit(1);
}

int main(int argc, char *argv[])
{
	int maxtime, ret=1;

	if (argc < 3) {
		usage();
		exit(1);
	}

#ifdef BSD_SETPGRP
	if (setpgrp(0,0) == -1) {
		perror("setpgrp");
		exit(1);
	}
#else
	if (setpgrp() == -1) {
		perror("setpgrp");
		exit(1);
	}
#endif

	maxtime = atoi(argv[1]);
	signal(SIGALRM, sig_alrm);
	alarm(maxtime);

	if (fork() == 0) {
		execvp(argv[2], argv+2);
		perror(argv[2]);
		exit(1);
	}

	do {
		int status;
		pid_t pid = wait(&status);
		if (pid != -1) {
			ret = WEXITSTATUS(status);
		} else if (errno == ECHILD) {
			break;
		}
	} while (1);

	exit(ret);
}
