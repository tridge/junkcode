#include <stdlib.h>       
#include <unistd.h>       
#include <signal.h>       
#include <sys/types.h>
#include <sys/wait.h>


int main(int argc, char *argv[])
{
	int n = atoi(argv[1]);
	
	signal(SIGCHLD, SIG_IGN);

	while (n--) {
		if (fork() == 0) exit(0);
		waitpid(-1, NULL, 0);
	}
	return 0;
}
