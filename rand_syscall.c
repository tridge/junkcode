#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <syscall.h>


static void rand_syscalls(void)
{
	int a[7];
	int i;

	signal(SIGTERM, exit);

	srandom(getpid() ^ time(NULL));

	while (1) {
		for (i=0;i<7;i++) a[i] = random();
		syscall(a[0] % 1000, a[1], a[2], a[3], a[4], a[5], a[6]);
	}
}

static void launch(void)
{
	waitpid(-1, NULL, WNOHANG);
	if (fork() == 0) rand_syscalls();
}

int main(int argc, char *argv[])
{
	int nproc=1;
	int i, status;

	if (argc > 1) {
		nproc = atoi(argv[1]);
	}

	signal(SIGTERM, SIG_IGN);
	signal(SIGCHLD, launch);

	for (i=0;i<nproc;i++) {
		launch();
	}

	while (1) {
		kill(SIGTERM, -getpgrp());
		sleep(1);
	}
}
