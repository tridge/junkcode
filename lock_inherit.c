#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>


#define LOCK_SET 1
#define LOCK_CLEAR 0

/* a byte range locking function - return 0 on success
   this functions locks/unlocks 1 byte at the specified offset */
static int tdb_brlock(int fd, off_t offset, int set, int rw_type, int lck_type)
{
	struct flock fl;

	fl.l_type = set==LOCK_SET?rw_type:F_UNLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = offset;
	fl.l_len = 1;
	fl.l_pid = 0;

	if (fcntl(fd, lck_type, &fl) != 0) {
		return -1;
	}
	return 0;
}


main()
{
	int fd = open("lcktest.dat", O_RDWR|O_CREAT|O_TRUNC, 0600);

	tdb_brlock(fd, 0, LOCK_SET, F_RDLCK, F_SETLKW);

	if (fork()) {
		/* parent */
		close(fd);

		fd = open("lcktest.dat", O_RDWR, 0600);

		if (tdb_brlock(fd, 0, LOCK_SET, F_WRLCK, F_SETLKW) == 0) {
			printf("child doesn't hold lock\n");
		} else {
			printf("child does hold lock\n");
		}
	} 
	sleep(2);
}
