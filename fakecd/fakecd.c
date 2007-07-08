/*
  a quick hack to allow ISO images to be used as a cdrom device in vmware.
  Waht we do is hook open() and ioctl() then for ioctl() calls on any file ending in ".iso" we 
  return enough garbage for vmware to think it has a real cdrom device 

  To install build fakecd.so then add it to /etc/ld.so.preload 
  In vmware set your cdrom device to whatever iso image you like. It
  _must_ have the extension ".iso"

  currenty only supports 1 faked cdrom drive

  tridge@linuxcare.com, October 2000
*/

#include <stdio.h>
#include <pwd.h>
#include <errno.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <linux/cdrom.h>

static int iso_fd = -1;

int open(const char *filename, int flags, mode_t mode)
{
	int ret;
	static int (*realopen)(const char *, int , mode_t );

	if (!realopen) {
		realopen = dlsym((void *)-1, "open");
	}
	
	ret = realopen(filename, flags, mode);

	if (ret != -1 && strcmp(filename+strlen(filename)-4, ".iso") == 0) {
		iso_fd = ret;
	}
	return ret;
}

int ioctl(int d, int request, void *arg)
{
	static int (*realioctl)(int , int , void *);

	if (!realioctl) {
		realioctl = dlsym((void *)-1, "ioctl");
	}

	if (d != iso_fd) {
		return realioctl(d, request, arg);
	}


	switch (request) {

	case CDROMVOLREAD:
	case CDROMVOLCTRL: {
		struct cdrom_volctrl *v = arg;
		memset(v, 0, sizeof(*v));
		return 0;
	}

	case CDROM_SET_OPTIONS: 
	case CDROM_CLEAR_OPTIONS:
		return CDC_LOCK;

	case CDROMREADTOCHDR: {
		struct cdrom_tochdr *v = arg;
		v->cdth_trk0 = 1;
		v->cdth_trk1 = 1;
		return 0;
	}

	case CDROMREADTOCENTRY: {
		struct cdrom_tocentry *v = arg;
		v->cdte_track=1;
		v->cdte_adr=1;
		v->cdte_ctrl=4;
		v->cdte_format=2;
		v->cdte_addr.lba=0;
		v->cdte_datamode=0;
		return 0;
	}

	case CDROMSUBCHNL:
	case CDROMMULTISESSION:
	case CDROM_DRIVE_STATUS:
	case CDROM_LOCKDOOR:
	case CDROM_MEDIA_CHANGED:
		return 0;
	default:
		/* fprintf(stderr,"fakecd: Unknown ioctl 0x%x\n", request); */
		break;
	}

	/* fall through to other ioctls */
	return realioctl(iso_fd, request, arg);
}
