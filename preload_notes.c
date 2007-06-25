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

#if 0
#define LOG_NAME "/tmp/notes.log"
#endif

static void xlogit(const char *fmt, ...)
{
#ifdef LOG_NAME
	static int logfd = -1;
	va_list ap;
	static int (*open_orig)(const char *, int, mode_t);

	if (!open_orig) {
		open_orig = dlsym(RTLD_NEXT, "open");
	}

	if (logfd == -1) {
		logfd = open_orig(LOG_NAME, O_WRONLY|O_APPEND|O_CREAT, 0666);
	}
	va_start(ap, fmt);
	vdprintf(logfd, fmt, ap);
	va_end(ap);
#endif
}


static const char *redirect_name(const char *filename)
{
	static char buf[1024];
	static int (*access_orig)(const char *, int );
	static const char *lroot;
	if (lroot == NULL) {
		lroot = getenv("LROOT");
	}
	if (lroot == NULL) {
		xlogit("You must set LROOT\n");
		abort();
	}
	if (filename[0] != '/' || strncmp(filename, lroot, strlen(lroot)) == 0) {
		return filename;
	}
	if (!access_orig) {
		access_orig = dlsym(RTLD_NEXT, "access");
	}
	snprintf(buf, sizeof(buf), "%s%s", lroot, filename);
	if (access_orig(buf, F_OK) == -1) {
		return filename;
	}
	
	xlogit("mapped '%s' to '%s'\n", filename, buf);

	return buf;
}

int setenv(const char *name, const char *value, int overwrite)
{
	static int (*setenv_orig)(const char *, const char *, int);
	int ret;

	if (!setenv_orig) {
		setenv_orig = dlsym(RTLD_NEXT, "setenv");
	}

	if (strcmp("LD_PRELOAD", name) == 0) {
		xlogit("skipping LD_PRELOAD of '%s'\n", value);
		return 0;
	}

#if 0
	if (strcmp("LD_LIBRARY_PATH", name) == 0) {
		xlogit("skipping LD_LIBRARY_PATH of '%s'\n", value);
		return 0;
	}
#endif

	ret = setenv_orig(name, value, overwrite);

	xlogit("setenv(\"%s\", \"%s\", %d) -> %d\n", 
		name, value, overwrite, ret);

	return ret;
}

void *dlopen(const char *filename, int flag)
{
	static void *(*dlopen_orig)(const char *, int);
	void *ret;

	if (!dlopen_orig) {
		dlopen_orig = dlsym(RTLD_NEXT, "dlopen");
	}

	ret = dlopen_orig(redirect_name(filename), flag);

	xlogit("dlopen(\"%s\", 0x%x) -> %p\n", filename, flag, ret);

	return ret;	
}


int open(const char *filename, int flags, ...)
{
	static int (*open_orig)(const char *, int, mode_t);
	int ret;
	va_list ap;
	mode_t mode;

	if (!open_orig) {
		open_orig = dlsym(RTLD_NEXT, "open");
	}

	va_start(ap, flags);
	mode = va_arg(ap, mode_t);
	va_end(ap);

	ret = open_orig(redirect_name(filename), flags, mode);

	xlogit("open(\"%s\", 0x%x, %o) -> %d\n", filename, flags, mode, ret);

	return ret;	
}

int stat(const char *filename, struct stat *st)
{
	static int (*stat_orig)(const char *, struct stat *);
	int ret;

	if (!stat_orig) {
		stat_orig = dlsym(RTLD_NEXT, "stat");
	}

	ret = stat_orig(redirect_name(filename), st);

	xlogit("stat(\"%s\") -> %d\n", filename, ret);

	return ret;	
}

int stat64(const char *filename, struct stat64 *st)
{
	static int (*stat64_orig)(const char *, struct stat64 *);
	int ret;

	if (!stat64_orig) {
		stat64_orig = dlsym(RTLD_NEXT, "stat64");
	}

	ret = stat64_orig(redirect_name(filename), st);

	xlogit("stat64(\"%s\") -> %d\n", filename, ret);

	return ret;	
}

FILE *fopen(const char *filename, const char *mode)
{
	static FILE *(*fopen_orig)(const char *, const char *);
	FILE *ret;

	if (!fopen_orig) {
		fopen_orig = dlsym(RTLD_NEXT, "fopen");
	}

	ret = fopen_orig(redirect_name(filename), mode);

	xlogit("fopen(\"%s\", \"%s\") -> %p\n", filename, mode, ret);

	return ret;	
}

int __xstat(int x, const char *filename, struct stat *st)
{
	static int (*__xstat_orig)(int x, const char *, struct stat *);
	int ret;

	if (!__xstat_orig) {
		__xstat_orig = dlsym(RTLD_NEXT, "__xstat");
	}

	ret = __xstat_orig(x, redirect_name(filename), st);

	xlogit("__xstat(\"%s\") -> %d\n", filename, ret);

	return ret;	
}

int access(const char *filename, int x)
{
	static int (*access_orig)(const char *, int );
	int ret;

	if (!access_orig) {
		access_orig = dlsym(RTLD_NEXT, "access");
	}

	ret = access_orig(redirect_name(filename), x);

	xlogit("access(\"%s\") -> %d\n", filename, ret);

	return ret;	
}

