#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <dlfcn.h>

int main(int argc, const char *argv[])
{
	const char *modpath = "public/module.so";
	void *handle;
	const char *(*version2)(void);
	const char *libversion(void);
	const char *res1, *res2;

	res1 = libversion();
	printf("Direct: libversion -> %s\n", res1);

	handle = dlopen(modpath, RTLD_NOW);
	if (handle == NULL) {
		printf("Failed to open '%s' - %s\n",
		       modpath, dlerror());
		exit(2);
	}

	version2 = dlsym(handle, "libversion");
	if (version2 == NULL) {
		printf("Failed to find libversion in %s\n", modpath);
		exit(2);
	}

	res2 = version2();

	printf("Module: libversion -> %s\n", res2);
	if (res1 != res2) {
		printf("ERROR: pointer mismatch\n");
		exit(1);
	}

#ifdef ENABLE_V2
	version2 = dlsym(handle, "libversion2");
	if (version2 == NULL) {
		printf("Failed to find libversion2 in %s\n", modpath);
		exit(2);
	}

	res2 = version2();

	printf("Module: libversion -> %s\n", res2);
	if (res1+1 != res2) {
		printf("ERROR: pointer mismatch\n");
		exit(1);
	}
#endif
	return 0;
}
