/* run a command with a limited timeout
   tridge@samba.org, January 2002
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static void usage(void)
{
	printf("
timed_command <time> <command>
");
}

int main(int argc, char *argv[])
{
	int maxtime;

	if (argc < 3) {
		usage();
		exit(1);
	}

	maxtime = strtol(argv[1], NULL, 0);
	alarm(maxtime);
	return execvp(argv[2], argv+2);
}
