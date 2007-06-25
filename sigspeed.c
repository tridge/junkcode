#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>

static int count;
static pid_t pid;

static void sigusr1(int num)
{
	count++;
	kill(pid, SIGUSR1);
	if (count % 1000000 == 0) {
		printf("%d\r", count);
		fflush(stdout);
	}
	if (count == 10000000) {
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	pid = getpid();
	signal(SIGUSR1, sigusr1);
	while (1) {
		kill(pid, SIGUSR1);
		pause();
	}
	return 0;
}
