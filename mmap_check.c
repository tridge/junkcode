#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/wait.h>

#ifndef MAP_FILE
#define MAP_FILE 0
#endif


static pid_t child_pid;

#define LENGTH (1<<20)
#define FNAME "mmap.dat"

static void child_main(int pfd)
{
	int fd;
	char *map;

	while ((fd = open(FNAME,O_RDWR)) == -1) sleep(1);

	map = mmap(0, LENGTH, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FILE, fd, 0);

	while (1) {
		int ofs, len, ret;
		int i;
		char buf[64];

		read(pfd, &ofs, sizeof(ofs));
		read(pfd, &len, sizeof(len));

		if (random() % 2) {
			memcpy(buf, map+ofs, len);
		} else {
			pread(fd, buf, len, ofs);
		}

		for (i=0;i<len;i++) {
			if (buf[i] != (char)(ofs+len+i)) {
				printf("child failed\n");
				exit(1);
			}
		}

		ret = 0;
		write(pfd, &ret, sizeof(ret));
	}
}

	
static void parent_main(int pfd)
{
	char *map;
	int fd = open(FNAME,O_RDWR|O_CREAT|O_TRUNC, 0600);

	ftruncate(fd, LENGTH);

	map = mmap(0, LENGTH, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FILE, fd, 0);

	while (1) {
		int ofs, len, ret;
		int i;
		char buf[64];

		ofs = random() % (LENGTH-64);
		len = random() % 64;

		for (i=0;i<len;i++) buf[i] = ofs+len+i;

		if (random() % 2) {
			memcpy(map+ofs, buf, len);
		} else {
			pwrite(fd, buf, len, ofs);
		}

		write(pfd, &ofs, sizeof(ofs));
		write(pfd, &len, sizeof(len));

		read(pfd, &ret, sizeof(ret));
	}
}



int main()
{
	int pfd[2];

	socketpair(AF_UNIX, SOCK_STREAM, 0, pfd);

	srandom(getpid());

	if ((child_pid=fork()) == 0) {
		close(pfd[0]);
		child_main(pfd[1]);
	} else {
		close(pfd[1]);
		parent_main(pfd[0]);
	}
	return 0;
}
