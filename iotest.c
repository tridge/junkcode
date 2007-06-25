#include <stdio.h>
#include <unistd.h>

main()
{
	int pid;

	if ((pid=fork()) != 0) {
		int i = 0;
		while (1) {
			printf("%d\n", i++);
			fflush(stdout);
			sleep(1);
		}
	}
	sleep(1000);
}
