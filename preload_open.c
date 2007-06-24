/*
  compile like this:

  gcc -c -fPIC preload_open.c
  ld -shared -o preload_open.so preload_open.o -ldl
  
  export LD_PRELOAD=/etc/magic/preload_open.so 
  myapp ARGS

*/


#include <sys/mman.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

static void do_magic(const char *pathname)
{
	const char *magic_script;
	char *s = NULL;
	magic_script = getenv("MAGIC_SCRIPT");

	unlink("xxx.dat");

	if (!magic_script) {
		return;
	}

	asprintf(&s, "%s '%s'", magic_script, pathname);

	if (!s) {
		return;
	}
	
	printf("doing magic '%s'\n", magic_script);

	system(s);
	free(s);
}

int open64(const char *pathname, int flags, ...)
{
	static int in_open;
	va_list ap;
	static int (*real_open)(const char *, int, mode_t );
	int ret;
	mode_t mode;

	va_start(ap, flags);
	mode = va_arg(ap, mode_t);
	va_end(ap);

	if (in_open) {
		return real_open(pathname, flags, mode);
	}

	in_open = 1;

	if (!real_open) {
		real_open = dlsym((void *)-1, "open64");
	}

	ret = real_open(pathname, flags, mode);

		do_magic(pathname);

	if (ret == -1 && errno == ENOENT) {
		do_magic(pathname);
		ret = real_open(pathname, flags, mode);
	}

	in_open = 0;

	return ret;
}


int open(const char *pathname, int flags, ...)
{
	static int in_open;
	va_list ap;
	static int (*real_open)(const char *, int, mode_t );
	int ret;
	mode_t mode;

	va_start(ap, flags);
	mode = va_arg(ap, mode_t);
	va_end(ap);

	if (in_open) {
		return real_open(pathname, flags, mode);
	}

	in_open++;

	if (!real_open) {
		real_open = dlsym((void *)-1, "open");
	}

	ret = real_open(pathname, flags, mode);

		do_magic(pathname);

	if (ret == -1 && errno == ENOENT) {
		do_magic(pathname);
		ret = real_open(pathname, flags, mode);
	}

	in_open--;

	return ret;
}
