#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <attr/xattr.h>


ssize_t sys_getxattr(const char *path, const char *name, void *value, size_t size)
{
	return getxattr(path, name, value, size);
}

ssize_t sys_fgetxattr(int filedes, const char *name, void *value, size_t size)
{
	return fgetxattr(filedes, name, value, size);
}

int sys_setxattr(const char *path, const char *name, const void *value, 
		 size_t size, int flags)
{
	return setxattr(path, name, value, size, flags);
}

int sys_fsetxattr(int filedes, const char *name, const void *value, 
		  size_t size, int flags)
{
	return fsetxattr(filedes, name, value, size, flags);
}


int main(int argc, const char *argv[])
{
	const char *fname = argv[1];
	int rc;
	size_t size = 1*1024*1024;
	char *buf = malloc(size);
	const char *attrname = "user.DOSATTRIB";

	memset(buf, 'x', size);

	do {
	  rc = sys_setxattr(fname, attrname, buf, size, 0);
	  if (rc == -1) size--;
	} while (rc == -1 && size > 1);
	if (rc == -1) {
		perror("setxattr");
		exit(1);
	}
	printf("set xattr of size %d\n", size);

	rc = sys_getxattr(fname, attrname, buf, size);
	if (rc == -1) {
		perror("getxattr");
		exit(1);
	}
	buf[rc] = 0;

	return 0;
}
