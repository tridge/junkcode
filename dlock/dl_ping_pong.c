/*
  this measures the ping-pong byte range lock latency. It is
  especially useful on a cluster of nodes sharing a common lock
  manager as it will give some indication of the lock managers
  performance under stress

  tridge@samba.org, February 2002

*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "dl_server.h"

static int port = DL_PORT;

/* run the ping pong test on fd */
static void ping_pong(struct dl_handle *handle, int num_locks)
{
	unsigned count = 0;
	int i=0;

	start_timer();

	lock_range(handle, 0, 1);
	i = 0;

	while (1) {
		if (lock_range(handle, (i+1) % num_locks, 1) != 0) {
			printf("lock at %d failed! - %s\n",
			       (i+1) % num_locks, strerror(errno));
		}
		if (unlock_range(handle, i, 1) != 0) {
			printf("unlock at %d failed! - %s\n",
			       i, strerror(errno));
		}
		i = (i+1) % num_locks;
		count++;
		if (end_timer() > 1.0) {
			printf("%8u locks/sec\r", 
			       (unsigned)(2*count/end_timer()));
			fflush(stdout);
			start_timer();
			count=0;
		}
	}
}

int main(int argc, char *argv[])
{
	char *server;
	int num_locks;
	struct dl_handle *handle;

	if (argc < 3) {
		printf("dl_ping_pong <server> <num_locks>\n");
		exit(1);
	}

	server = argv[1];
	num_locks = atoi(argv[2]);

	handle = dlock_open(server, port);
	if (!handle) {
		fatal("failed to open link to dlock server\n");
	}

	ping_pong(handle, num_locks);

	return 0;
}
