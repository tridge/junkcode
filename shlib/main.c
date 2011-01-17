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

	printf("Direct: libversion -> %s\n", libversion());

	handle = dlopen(modpath, RTLD_NOW);
	if (handle == NULL) {
		printf("Failed to open '%s' - %s\n",
		       modpath, dlerror());
		exit(1);
	}

	version2 = dlsym(handle, "libversion");
	if (version2 == NULL) {
		printf("Failed to find libversion in %s\n", modpath);
		exit(1);
	}

	printf("Module: libversion -> %s\n", version2());
	return 0;
}
