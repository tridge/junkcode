#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/inotify.h>
#include <errno.h>
#include <asm/unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#if 0
_syscall0(int, inotify_init);
_syscall3(int, inotify_add_watch, int, fd, const char *, path, __u32, mask);
_syscall2(int, inotify_rm_watch, int, fd, int, wd);

#else

static int inotify_init(void)
{
	return syscall(__NR_inotify_init);
}

static int inotify_add_watch(int fd, const char *path, __u32 mask)
{
	static int xx;
	return syscall(__NR_inotify_add_watch, fd, path, mask+(++xx));
}

static int inotify_rm_watch(int fd, int wd)
{
	return syscall(__NR_inotify_rm_watch, fd, wd);
}
#endif

static void usage(void)
{
	printf("usage: inotify <DIRECTORY>\n");
}

int main(int argc, const char *argv[])
{
	const char *path;
	int fd, wd1, wd2;
	pid_t child;

	if (argc < 2) {
		usage();
		exit(1);
	}

	path = argv[1];

	if ((child = fork()) == 0) {
		char *testname;
		asprintf(&testname, "%s/inotify_test.dat", path);
		while (1) {
			close(open(testname, O_CREAT|O_RDWR, 0666));
			unlink(testname);
			sleep(1);
		}
	}

	fd = inotify_init();
	wd1 = inotify_add_watch(fd, path, IN_CREATE|IN_DELETE);
	wd2 = inotify_add_watch(fd, path, IN_CREATE|IN_DELETE);

	while (1) {
		char buf[1024];
		int ret = read(fd, buf, sizeof(buf));
		char *p = buf;
		struct inotify_event *ev;
		while (ret > sizeof(ev)) {
			ev = (struct inotify_event *)p;
			printf("%d 0x%08x %d '%s'\n", 
			       ev->wd, ev->mask, ev->len, ev->name);
			p += sizeof(*ev) + ev->len;
			ret -= sizeof(*ev) + ev->len;
		}
	}

	inotify_rm_watch(fd, wd1);
	inotify_rm_watch(fd, wd2);
	close(fd);

	kill(child, SIGKILL);

	return 0;
}
