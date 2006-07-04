/*
  preload hack to catch all filename ops

  tridge@samba.org July 2006
*/
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
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>

static void xlogit(const char *fmt, ...)
{
	static int initialised;
	static int logfd = -1;
	va_list ap;
	static int (*open_orig)(const char *, int, mode_t);
	static char *(*getenv_orig)(const char *);

	if (logfd == -1 && !initialised) {
		const char *log_name;
		initialised = 1;
		getenv_orig = dlsym(RTLD_NEXT, "getenv");
		open_orig = dlsym(RTLD_NEXT, "open");
		log_name = getenv_orig("FILEOPS_LOG");
		if (log_name != NULL) {
			logfd = open_orig(log_name, O_WRONLY|O_APPEND|O_CREAT, 0666);
		}
	}

	if (logfd != -1) {
		va_start(ap, fmt);
		vdprintf(logfd, fmt, ap);
		va_end(ap);
	}
}

static void xlogerror(int ret)
{
	if (ret == -1) {
		xlogit("error=%s\n", strerror(errno));
	}
}

static void xlogerror_p(void *p)
{
	if (p == NULL) {
		xlogit("error=%s\n", strerror(errno));
	}
}

static const char *redirect_name(const char *name)
{
	return name;
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

	ret = setenv_orig(name, value, overwrite);

	xlogit("setenv(\"%s\", \"%s\", %d) -> %d\n", 
		name, value, overwrite, ret);

	return ret;
}

char *getenv(const char *name)
{
	static char *(*getenv_orig)(const char *);
	char *ret;

	if (!getenv_orig) {
		getenv_orig = dlsym(RTLD_NEXT, "getenv");
	}

	ret = getenv_orig(name);

	xlogit("getenv(\"%s\") -> '%s'\n", name, ret);

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
	xlogerror(ret);

	return ret;	
}

int open64(const char *filename, int flags, ...)
{
	static int (*open64_orig)(const char *, int, mode_t);
	int ret;
	va_list ap;
	mode_t mode;

	if (!open64_orig) {
		open64_orig = dlsym(RTLD_NEXT, "open64");
	}

	va_start(ap, flags);
	mode = va_arg(ap, mode_t);
	va_end(ap);

	ret = open64_orig(redirect_name(filename), flags, mode);

	xlogit("open64(\"%s\", 0x%x, %o) -> %d\n", filename, flags, mode, ret);
	xlogerror(ret);

	return ret;	
}

int __open(const char *filename, int flags, ...)
{
	static int (*__open_orig)(const char *, int, mode_t);
	int ret;
	va_list ap;
	mode_t mode;

	if (!__open_orig) {
		__open_orig = dlsym(RTLD_NEXT, "__open");
	}

	va_start(ap, flags);
	mode = va_arg(ap, mode_t);
	va_end(ap);

	ret = __open_orig(redirect_name(filename), flags, mode);

	xlogit("__open(\"%s\", 0x%x, %o) -> %d\n", filename, flags, mode, ret);
	xlogerror(ret);

	return ret;	
}

int __open64(const char *filename, int flags, ...)
{
	static int (*__open64_orig)(const char *, int, mode_t);
	int ret;
	va_list ap;
	mode_t mode;

	if (!__open64_orig) {
		__open64_orig = dlsym(RTLD_NEXT, "__open64");
	}

	va_start(ap, flags);
	mode = va_arg(ap, mode_t);
	va_end(ap);

	ret = __open64_orig(redirect_name(filename), flags, mode);

	xlogit("__open64(\"%s\", 0x%x, %o) -> %d\n", filename, flags, mode, ret);
	xlogerror(ret);

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
	xlogerror(ret);

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
	xlogerror(ret);

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
	xlogerror_p(ret);

	return ret;	
}

void *gzopen(const char *filename, const char *mode)
{
	static void *(*gzopen_orig)(const char *, const char *);
	void *ret;

	if (!gzopen_orig) {
		gzopen_orig = dlsym(RTLD_NEXT, "gzopen");
	}

	ret = gzopen_orig(redirect_name(filename), mode);

	xlogit("gzopen(\"%s\", \"%s\") -> %p\n", filename, mode, ret);
	xlogerror_p(ret);

	return ret;	
}

DIR *opendir(const char *filename)
{
	static DIR *(*opendir_orig)(const char *);
	DIR *ret;

	if (!opendir_orig) {
		opendir_orig = dlsym(RTLD_NEXT, "opendir");
	}

	ret = opendir_orig(redirect_name(filename));

	xlogit("opendir(\"%s\") -> %p\n", filename, ret);
	xlogerror_p(ret);

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
	xlogerror(ret);

	return ret;	
}

int __xstat64(int x, const char *filename, struct stat64 *st)
{
	static int (*__xstat64_orig)(int x, const char *, struct stat64 *);
	int ret;

	if (!__xstat64_orig) {
		__xstat64_orig = dlsym(RTLD_NEXT, "__xstat64");
	}

	ret = __xstat64_orig(x, redirect_name(filename), st);

	xlogit("__xstat64(\"%s\") -> %d\n", filename, ret);
	xlogerror(ret);

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
	xlogerror(ret);

	return ret;	
}


int execve(const char *filename, char *const argv[], char *const envp[])
{
	static int (*execve_orig)(const char *, char *const argv[], char *const envp[] );

	if (!execve_orig) {
		execve_orig = dlsym(RTLD_NEXT, "execve");
	}

	xlogit("execve(\"%s\")\n", filename);

	return execve_orig(redirect_name(filename), argv, envp);
}


int execvp(const char *filename, char *const argv[])
{
	static int (*execvp_orig)(const char *, char *const argv[]);

	if (!execvp_orig) {
		execvp_orig = dlsym(RTLD_NEXT, "execvp");
	}

	xlogit("execvp(\"%s\")\n", filename);

	return execvp_orig(redirect_name(filename), argv);
}


int execv(const char *filename, char *const argv[])
{
	static int (*execv_orig)(const char *, char *const argv[]);

	if (!execv_orig) {
		execv_orig = dlsym(RTLD_NEXT, "execv");
	}

	xlogit("execv(\"%s\")\n", filename);

	return execv_orig(redirect_name(filename), argv);
}



