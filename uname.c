#include <pwd.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <dlfcn.h>

int uname(struct utsname *buf)
{
	static int (*uname_orig)(struct utsname *buf);
	if (!uname_orig) {
		uname_orig = dlsym(-1, "uname");
	}
	uname_orig(buf);
	strcpy(buf->nodename, "jeremy");
	return 0;
}
