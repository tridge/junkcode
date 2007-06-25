#include <stdlib.h>
#include <fcntl.h>

#define LEN 0x1000
#define ADDR 0x0

int main()
{
	int fdpair[2];
	int fd = open("mem.dat", O_WRONLY|O_CREAT|O_TRUNC, 0600);

	if (fd == -1) {
		perror("open");
		exit(1);
	}

	while (pipe(fdpair) == 0) {
		if (fork()) {
			char buf[LEN];
			if (read(fdpair[0], buf, LEN) != LEN) {
				perror("read");
				exit(1);
			}
			if (write(fd,buf,LEN) != LEN) {
				perror("write");
				exit(1);
			}
			waitpid(-1, NULL, 0);
		} else {
			if (write(fdpair[1],ADDR,LEN) != LEN) {
				perror("write");
				exit(1);
			}
			_exit(0);
		}
	}

	return 0;
}
