#include <stdio.h>
#include <sys/types.h>
#include <dlfcn.h>

void *shmat(int shmid, const void *shmaddr, int shmflag)
{
	static void *h;
	static void *(*shmat_orig)(int , const void *, int );
	static int count;
	if (!h) {
		h = dlopen("/lib/libc.so.6", RTLD_LAZY);
		shmat_orig = dlsym(h, "shmat");
		count = atoi(getenv("SHMAT_COUNT"));
	}

	fprintf(stderr,"shmat(%d, %p, 0x%x)\n", shmid, shmaddr, shmflag);

	if (! count) return NULL;

	count--;

	return shmat_orig(shmid, shmaddr, shmflag);
}
