#include <sched.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>

int main(int argc, const char *argv[])
{
	pid_t pid;
	struct sched_param p;

	if (argc < 2) {
		fprintf(stderr, "usage: setsched <pid>\n");
		exit(1);
	}

	p.__sched_priority = 1;

	pid = atoi(argv[1]);
	if (sched_setscheduler(pid, SCHED_FIFO, &p) == -1) {
		perror("sched_setscheduler");
		return -1;
	}
	return 0;
}
