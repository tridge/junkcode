#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	int i, n = atoi(argv[1]);
	pid_t *pids = malloc(n * sizeof(pid_t));

	for (i=0;i<n;i++) {
		pids[i] = fork();
		if (pids[i] == 0) {
			sleep(1000);
			exit(0);
		}
	}


	for (i=0;i<n;i++) {
		waitpid(pids[i], NULL, 0);
	}
	return 0;
}
