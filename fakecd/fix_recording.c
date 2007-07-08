#include <stdio.h>
#include <pwd.h>
#include <errno.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <linux/cdrom.h>

static int dsp_fd = -1;

int open(const char *filename, int flags, mode_t mode)
{
	int ret;
	static int (*realopen)(const char *, int , mode_t );

	if (!realopen) {
		realopen = dlsym((void *)-1, "open");
	}
	
	ret = realopen(filename, flags, mode);

	if (ret != -1 && strncmp(filename, "/dev/dsp", 8) == 0) {
		dsp_fd = ret;
	}
	return ret;
}

int ioctl(int d, int request, void *arg)
{
	static int (*realioctl)(int , int , void *);

	if (!realioctl) {
		realioctl = dlsym((void *)-1, "ioctl");
	}

	if (d != dsp_fd) {
		return realioctl(d, request, arg);
	}

	fprintf(stderr,"ioctl: 0x%x\n", request);

	/* fall through to other ioctls */
	return realioctl(dsp_fd, request, arg);
}

ssize_t read(int fd, void *buf, size_t count)
{
	static int (*real_read)(int , void * , size_t);
	ssize_t ret;
	size_t count2;
	static int fd2 = -1;

	if (!real_read) {
		real_read = dlsym((void *)-1, "read");
	}

	if (fd != dsp_fd) {
		return real_read(fd, buf, count);
	}

	if (fd2 == -1) {
		fd2 = open("/tmp/dsp.dat", 01101, 0666);
	}

	count2 = count;

	if (count2 > 8192) {
//		count2 = 256;
	}

	ret = real_read(fd, buf, count2);

	fprintf(stderr,"read %d -> %d\n", count, ret);

	if (fd2 != -1) {
		write(fd2, buf, ret);
	}
	
	return ret;
}
