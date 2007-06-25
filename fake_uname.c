#include <pwd.h>
#include <sys/types.h>
#include <sys/utsname.h>

int uname(struct utsname *buf)
{

}

struct passwd *getpwuid(uid_t uid)
{
	static void *h;
	static struct passwd * (*getpwuid_orig)(uid_t uid);
	if (!h) {
		h = dlopen("/lib/libc.so.6", RTLD_LAZY);
		getpwuid_orig = dlsym(h, "getpwuid");
	}
	
	return getpwuid_orig(uid+1);
}
