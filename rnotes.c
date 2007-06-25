#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

int main(void)
{
	const char *lroot = getenv("LROOT");
	uid_t uid = getuid();
	if (setuid(0) != 0) return -1;
	if (chroot(lroot) != 0) return -1;
	if (chdir("/") != 0) return -1;
	if (setuid(uid) != 0) return -1;

	return system("/usr/bin/richclient");
}
