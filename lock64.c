#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>


#define fcntl fcntl64
#undef F_SETLKW 
#undef F_SETLK
#define F_SETLK 13
#define F_SETLKW 14


int fcntl64(int fd, int cmd, struct flock * lock)
{
	return syscall(221, fd, cmd, lock);
}

int main(int argc, char *argv[])
{
	struct flock fl;
	int fd, ret;

	printf("sizeof(fl.l_start)=%d\n", sizeof(fl.l_start));
	fd = open("lock64.dat", O_RDWR|O_CREAT);

	fl.l_type = F_RDLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 16;
	fl.l_len = 1;
	fl.l_pid = 0;

	ret = fcntl(fd, F_SETLKW, &fl);

	printf("ret=%d pid=%d\n", ret, getpid());

	sleep(30);
	return 0;
}
