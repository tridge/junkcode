#include <pwd.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <stdio.h>

const char *getenv(const char *name)
{
	static void *h;
	static const char *(*getenv_orig)(const char *);
	const char *ret;

	if (!h) {
		h = dlopen("/lib/libc.so.6", RTLD_LAZY);
		getenv_orig = dlsym(h, "getenv");
	}

	ret = getenv_orig(name);

	fprintf(stderr, "getenv(%s) -> '%s'\n", name, ret);

	return ret;
}
