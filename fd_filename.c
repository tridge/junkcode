#include <sys/types.h>
#include <utime.h>
#include <stdio.h>
#include <errno.h>



int fd_utime(int fd, struct utimbuf *buf)
{
	char *fd_path = NULL;
	int ret;

	asprintf(&fd_path, "/proc/self/%d", fd);
	if (!fd_path) {
		errno = ENOMEM;
		return -1;
	}

	ret = utime(fd_path, buf);
	free(fd_path);
	return ret;
}
