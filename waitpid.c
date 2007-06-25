#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>

x_waitpid(pid_t pid, int *status, int options)
{

}

main()
{
	pid_t pid1;

	if ((pid1=fork()) == 0) {
		/* first child */
		sleep(5);
		exit(3);
	}
	

	if (fork() == 0) {
		int status = 0;
		pid_t pid;
		/* second child */
		pid = waitpid(-getpgrp(), &status, 0);
		printf("got exit status %d from child %d pid1=%d\n", 
		       WEXITSTATUS(status), pid, pid1);
		exit(0);
	}
	sleep(10);
}
