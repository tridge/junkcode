#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{
	volatile int *buf;
	int size;
	pid_t child, parent;

	if (argc < 2) {
		printf("shm_sample <size>\n");
		exit(1);
	}

	size = atoi(argv[1]);

	buf = (int *)mmap(0, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED,
			  -1, 0);
	if (buf == (int *)-1) {
		perror("mmap");
		exit(1);
	}

	parent = getpid();
	child = fork();

	if (!child) {
		buf[0] = parent;
		return 0;
	}

	waitpid(child, 0, 0);

	if (buf[0] != parent) {
		printf("memory not shared? (buf[0]=%d parent=%d)\n", buf[0], parent);
	} else {
		printf("shared OK\n");
	}

	return 0;
}

