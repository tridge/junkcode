#define _GNU_SOURCE
#include <stdio.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <wchar.h>

FILE *xxfopen1(const char *filename, const char *mode)
{
	FILE *(*fopen_orig)(const char *, const char *);
	void *dl = dlopen("/lib/libc.so.6", RTLD_NOW);
	fopen_orig = dlsym(dl, "fopen");
	printf("fopen: %p  fopen_orig_dlopen: %p\n", fopen, fopen_orig);
	return fopen_orig(filename, mode);
}

FILE *xxfopen2(const char *filename, const char *mode)
{
	FILE *(*fopen_orig)(const char *, const char *);
	FILE *(*fopen_orig2)(const char *, const char *);
	fopen_orig = dlsym(RTLD_NEXT, "fopen");
	fopen_orig2 = dlsym(RTLD_NEXT, "fopen");
	printf("fopen: %p  fopen_orig_next: %p %p\n", fopen, fopen_orig, fopen_orig2);
	return fopen_orig(filename, mode);
}

int main(void)
{
	FILE *f1, *f2, *f3;
	int ret1, ret2, ret3;
	char *s;
	
	f1 = xxfopen1("test.txt", "w");
	f2 = xxfopen2("test.txt", "w");
	f3 = fopen("test.txt", "w");
	ret1 = fwide(f1, 1);
	ret2 = fwide(f2, 1);
	ret3 = fwide(f3, 1);

	printf("f1 %d  f2 %d f3 %d\n", ret1, ret2, ret3);

	asprintf(&s, "cat /proc/%d/maps", getpid());
	system(s);

	return 0;
}
