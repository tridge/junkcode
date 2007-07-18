#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

int mkdir(const char *pathname, mode_t mode)
{
	static int (*mkdir_orig)(const char *pathname, mode_t mode);
	int logfd;
	struct stat st;
	if (!mkdir_orig) {
		mkdir_orig = dlsym(RTLD_NEXT, "mkdir");
	}
	logfd = open("/tmp/mkdir.log", O_APPEND|O_WRONLY|O_CREAT, 0666);
	if (logfd != -1) {
		dprintf(logfd, "%-8s %s\n", 
			stat(pathname, &st) == 0? "exist":"notexist",
			pathname);
		close(logfd);
	}
	return mkdir_orig(pathname, mode);
}
