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
	uid_t id, id2;
	int fd;
	struct stat st;

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

	id2 = geteuid();
	if (id2 != id) {
		fprintf(stderr, "geteuid gave wrong id (%d)", id2);
		return -1;
	}
	
	unlink(name);

	fd = open(name, O_CREAT|O_EXCL|O_RDWR, 0644);
	if (fd == -1) {
		perror("open");
		return -1;
	}

	fstat(fd, &st);

	if (st.st_uid != id) {
		printf("Created with wrong uid (%d should be %d)!\n",
		       st.st_uid, id);
		exit(1);
	}

	close(fd);
	return 0;
}

