#include <sys/mman.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <fcntl.h>

void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
	static void *h;
	static void *(*mmap_orig)(void *, size_t , int , int , int, off_t );
	static int log_fd;
	static char buf[200];
	int n;
	void *ret;

	if (!h) {
		h = dlopen("/lib/libc.so.6", RTLD_LAZY);
		mmap_orig = dlsym(h, "mmap");
		log_fd = open("/tmp/mmap.log", O_WRONLY|O_CREAT|O_APPEND, 0666);
	}

	ret = mmap_orig(start, length, prot, flags, fd, offset);

	n = snprintf(buf, sizeof(buf), "mmap(%p, 0x%x, 0x%x, 0x%x, %d, 0x%x) = %p\n", 
		     start, length, prot, flags, fd, offset, ret);
	
	write(log_fd, buf, n);

	return ret;
}
