/*
  compile like this:

  gcc -c -fPIC preload_open.c
  ld -shared -o preload_open.so preload_open.o -ldl
  
*/

#define _GNU_SOURCE
#include <sys/mman.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>

static int usb_fd = -1;
static const char *usb_path = "/dev/vboxusb";

#define LOG_NAME "/tmp/usb.log"

static int (*real_open)(const char *, int, mode_t );

static void logit(const char *fmt, ...)
{
	static int logfd = -1;
	va_list ap;

	if (!real_open) {
		real_open = dlsym(RTLD_NEXT, "open");
	}

	if (logfd == -1) {
		logfd = real_open(LOG_NAME, O_WRONLY|O_APPEND|O_CREAT, 0666);
	}
	va_start(ap, fmt);
	vdprintf(logfd, fmt, ap);
	va_end(ap);
}

int open(const char *pathname, int flags, ...)
{
	va_list ap;
	int ret;
	mode_t mode;

	va_start(ap, flags);
	mode = va_arg(ap, mode_t);
	va_end(ap);

	if (usb_path == NULL) {
		usb_path = getenv("USB_PATH");
	}

	if (!real_open) {
		real_open = dlsym(RTLD_NEXT, "open");
	}

	ret = real_open(pathname, flags, mode);
	if (ret != -1 && usb_path && strncmp(pathname, usb_path, strlen(usb_path)) == 0) {
		logit("(%4u) opening %s as fd=%d\n", getpid(), pathname, ret);
		usb_fd = ret;
	}
	return ret;
}

int open64(const char *pathname, int flags, ...)
{
	va_list ap;
	int ret;
	mode_t mode;

	va_start(ap, flags);
	mode = va_arg(ap, mode_t);
	va_end(ap);

	if (usb_path == NULL) {
		usb_path = getenv("USB_PATH");
	}

	if (!real_open) {
		real_open = dlsym(RTLD_NEXT, "open64");
	}

	ret = real_open(pathname, flags, mode);
	if (ret != -1 && usb_path && strncmp(pathname, usb_path, strlen(usb_path)) == 0) {
		logit("(%4u) opening %s as fd=%d\n", getpid(), pathname, ret);
		usb_fd = ret;
	}
	return ret;
}

int close(int fd)
{
	static int (*real_close)(int fd);
	int ret;

	if (!real_close) {
		real_close = dlsym(RTLD_NEXT, "close");
	}

	ret = real_close(fd);
	if (ret == 0 && fd == usb_fd) {
		logit("(%4u) closing fd=%d\n", getpid(), fd);
		usb_fd = -1;
	}
	return ret;
}


int ioctl(int fd, unsigned long int cmd, ...)
{
	va_list ap;
	void *arg;
	static int (*real_ioctl)(int fd, int cmd, void *arg);
	int ret;

	if (!real_ioctl) {
		real_ioctl = dlsym(RTLD_NEXT, "ioctl");
	}

	va_start(ap, cmd);
	arg = va_arg(ap, void *);
	va_end(ap);

	ret = real_ioctl(fd, cmd, arg);

	if (ret == 0 && fd == usb_fd && 
	    (cmd == USBDEVFS_REAPURBNDELAY) && arg != NULL) {
		struct usbdevfs_urb *urb = *(struct usbdevfs_urb **)arg;
		uint8_t *data = urb->buffer;
		uint8_t newdata[14];
		FILE *df;
		int i;
		if (urb->actual_length != sizeof(newdata)) {
			logit("actual_length=%u\n", urb->actual_length);
			return ret;
		}
		for (i=0;i<urb->actual_length;i++) {
			logit("%02X ", data[i]);
		}
		logit("\n");
		df = fopen("/tmp/usb.data", "r");
		if (df == NULL) {
			return ret;
		}
		for (i=0;i<sizeof(newdata);i++) {
			unsigned v;
			if (fscanf(df, "%02X", &v) != 1) {
				fclose(df);
				return ret;
			}
			newdata[i] = v;
		}
		fclose(df);
		memcpy(urb->buffer, newdata, sizeof(newdata));
		logit("(%4u) replaced actual_length=%u\n", getpid(), urb->actual_length);
	}

	return ret;
}
