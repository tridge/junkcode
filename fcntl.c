#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) 
{
	struct flock lock;
        int status;
	int fd = open("conftest.dat", O_CREAT|O_RDWR, 0600);
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 1;
	lock.l_pid = 0;
	
	fcntl(fd,F_SETLK,&lock);
	if (fork() == 0) {
		lock.l_start = 1;		
		exit(fcntl(fd,F_SETLK,&lock) != 0);
        }
        wait(&status);
	unlink("conftest.dat");
	if (WEXITSTATUS(status)) {
		perror("lock");
		exit(1);
	} else {
		printf("OK\n");
	}
	exit(WEXITSTATUS(status));
}
