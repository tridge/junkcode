/*
  pread_cache.c: a cache to map pread() calls to fread() to prevent small
  reads on MacOSX network filesystems that don't do block cacheing

  tridge@samba.org January 2008

  released under GNU GPLv3 or later
  
  to compile on MacOSX:
    gcc pread.c -Wall -g -o pread.dylib -dynamiclib 

  usage:

    DYLD_INSERT_LIBRARIES=/Users/tridge/pread.dylib DYLD_FORCE_FLAT_NAMESPACE=1 command

*/
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <errno.h>

/* chosen to match the smbfs on MacOSX read size */
#define MAX_READ 61000

/* to keep the data structures simple, we only cache the low MAX_FILES file descriptors */
#define MAX_FILES 1024
static FILE *file_handles[MAX_FILES];
static unsigned char file_state[MAX_FILES];

#define STATE_NONE    0
#define STATE_CACHED  1
#define STATE_DISABLE 2

/* catch close to free up any existing caches */
int close(int fd)
{
	static int (*close_orig)(int fd);
	if (close_orig == NULL) {
		close_orig = dlsym(RTLD_NEXT, "close");
		if (close_orig == NULL) {
			abort();
		}
	}
	if (fd < 0 || fd >= MAX_FILES) {
		return close_orig(fd);
	}

	if (file_state[fd] == STATE_CACHED) {
		FILE *f;
		file_state[fd] = STATE_NONE;
		f = file_handles[fd];
		file_handles[fd] = NULL;
		/* the fclose() does an implied close() */
		return fclose(f);
	}

	file_state[fd] = STATE_NONE;
	return close_orig(fd);
}

/*
  catch pread() and if possible map to fread()
 */
ssize_t pread(int fd, void *buf, size_t nbytes, off_t offset)
{
	static ssize_t (*pread_orig)(int d, void *buf, size_t nbytes, off_t offset);
	static int pagesize;

	if (pread_orig == NULL) {
		pread_orig = dlsym(RTLD_NEXT, "pread");
		if (pread_orig == NULL) {
			abort();
		}
		pagesize = getpagesize();
	}

	/* large reads and reads on out of range file descriptors are not cached */
	if (nbytes >= MAX_READ || fd < 0 || fd >= MAX_FILES) {
		return pread_orig(fd, buf, nbytes, offset);
	}

	if (file_state[fd] == STATE_DISABLE) {
		return pread_orig(fd, buf, nbytes, offset);		
	}

	/* see if we already have a cache */
	if (file_state[fd] == STATE_NONE) {
		void *ptr;

		/* see if the file was opened read-write. This relies on the mmap error codes */
		ptr = mmap(NULL, pagesize, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, fd, 0);
		if (ptr != (void *)-1) {
			munmap(ptr, pagesize);
			file_state[fd] = STATE_DISABLE;
			return pread_orig(fd, buf, nbytes, offset);		
		}
		if (errno != EACCES) {
			file_state[fd] = STATE_DISABLE;
			return pread_orig(fd, buf, nbytes, offset);
		}

		file_handles[fd] = fdopen(fd, "r");
		if (file_handles[fd] == NULL) {
			file_state[fd] = STATE_DISABLE;
			return pread_orig(fd, buf, nbytes, offset);
		}

		/* ensure we have a big enough buffer */
		setvbuf(file_handles[fd], NULL, _IOFBF, MAX_READ);

		file_state[fd] = STATE_CACHED;
	}

	/* seek to the right place if need be */
	if (ftello(file_handles[fd]) != offset &&
	    fseeko(file_handles[fd], offset, SEEK_SET) != 0) {
		return pread_orig(fd, buf, nbytes, offset);
	}

	return fread(buf, 1, nbytes, file_handles[fd]);
}

