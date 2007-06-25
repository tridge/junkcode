#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, const char *argv[])
{
	const char *name;
	uid_t id;
	int fd;

	if (argc < 3) {
		printf("id_create <name> <uid>\n");
		exit(1);
	}

	name = argv[1];
	id = atoi(argv[2]);

	if (seteuid(id) != 0) {
		perror("seteuid");
		return -1;
	}
	
	unlink(name);
	fd = open(name, O_CREAT|O_EXCL|O_RDWR, 0644);
	if (fd == -1) {
		perror("open");
		return -1;
	}

	close(fd);
	return 0;
}

