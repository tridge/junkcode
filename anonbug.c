#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{
	int *buf;
	int size;
	pid_t child, parent;

	if (argc < 2) {
		printf("shm_sample <size>\n");
		exit(1);
	}

	size = atoi(argv[1]);

	buf = (int *)mmap(0, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
	if (buf == (int *)-1) {
		perror("mmap failed\n");
		exit(1);
	}

	buf[0] = -1;
	parent = getpid();

	child = fork();
	if (child == -1) {
		perror("fork");
		exit(1);
	}

	if (child == 0) {
		buf[0] = parent;
		return 0;
	}

	waitpid(child, 0, 0);

	if (buf[0] != parent) {
		printf("memory not shared? (%d != %d)\n", buf[0], parent);
	} else {
		printf("shared memory OK (%d == %d)\n", buf[0], parent);
	}

	return 0;
}
