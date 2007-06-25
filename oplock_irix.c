#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/capability.h>

#define FNAME "test.dat"

static int fd;
static int have_lease;
static int got_signal;

static void sig_usr1(int sig)
{
	got_signal = 1;
}

int main(int argc, char *argv[])
{
	int pfd[2];
	cap_t cap = cap_get_proc();

	cap->cap_effective |= CAP_NETWORK_MGT;
	if (cap_set_proc(cap) == -1) failed("cap_set_proc");
	cap_free(cap);

	if (pipe(pfd) != 0) failed("pipe");

	if ((fd = open(FNAME, O_RDWR|O_CREAT|O_EXCL|O_TRUNC, 0600)) == -1) failed("open");

	if (fcntl(fd, F_SETSIG, SIGUSR1) == -1) failed("setsig");

	signal(SIGUSR1, sig_usr1);

	while (1) {
		sleep(1);

		if (got_signal) {
			if (fcntl(fd, F_OPLKACK, OP_REVOKE) == -1) failed("revoke");
			got_signal = 0;
			have_lease = 0;
		}

		if (!have_lease && fcntl(fd, F_OPLKREG, pfd[1]) == -1) continue;
		have_lease = 1;
	}

	return 0;
}

