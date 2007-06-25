#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/inotify.h>
#include <errno.h>
#include <asm/unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

int main(void)
{
	int fd = open("xx.dat", O_CREAT, 0666);

	if (fork() == 0) {
		lseek(fd, 10, SEEK_SET);
		_exit(0);
	}

	printf("ofs %d\n", lseek(fd, 0, SEEK_CUR));
	return 0;
}
