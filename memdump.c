#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <fcntl.h>

#define LEN 0x1000
#define ADDR 0x0
#define SIZE 8*1024

int main()
{
	int fdpair[2];
	int fd, i=0;
	int mapfd, status;
	char *map;
	char buf[LEN];
	char lastbuf[LEN];

	fd = open("mem.dat", O_WRONLY|O_CREAT|O_TRUNC, 0600);
	if (fd == -1) {
		perror("open");
		exit(1);
	}

	mapfd = open("/dev/zero", O_RDWR);
	if (mapfd == -1) {
		perror("open");
		exit(1);
	}

	map = mmap(0, SIZE*LEN, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FILE,
		   mapfd, 0);

	while (i < SIZE && pipe(fdpair) == 0) {
		if (fork()) {
			memset(buf,'Z', LEN);
			if (read(fdpair[0], buf, LEN) != LEN) {
				perror("read");
				exit(1);
			}
			if (memcmp(lastbuf,buf,LEN)) {
				if (write(fd,buf,LEN) != LEN) {
					perror("write");
					exit(1);
				} else {
					printf(".");
					fflush(stdout);
				}
			}
			memcpy(lastbuf, buf, LEN);
			waitpid(-1, &status, 0);
		} else {
			if (write(fdpair[1],ADDR,LEN) != LEN) {
				perror("write");
				exit(1);
			}
			_exit(0);
		}
		close(fdpair[0]);
		close(fdpair[1]);

		map[i*LEN] = i++;
	}

	return 0;
}
