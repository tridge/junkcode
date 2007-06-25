#include <unistd.h>
#include <fcntl.h>

/* lock a byte range in a open file */
int lock_range(int fd, int offset, int len)
{
	struct flock lock;

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = offset;
	lock.l_len = len;
	lock.l_pid = 0;
	
	return fcntl(fd,F_SETLK,&lock) == 0;
}

int main(int argc, char *argv[])
{
	char *cmd, *lockf;
	int fd;
	struct flock lock;

	if (argc < 3) {
		printf("lockrun <lockfile> <cmd>\n");
		exit(1);
	}

	lockf = argv[1];
	cmd = argv[2];

	fd = open(lockf, O_CREAT|O_TRUNC|O_RDWR, 0600);
	if (fd == -1) exit(1);

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 1;
	lock.l_pid = 0;

	if (fcntl(fd,F_SETLKW,&lock) == 0) {
		system(cmd);
	}
	return 0;
}
