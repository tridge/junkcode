#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/statvfs.h>

int main(int argc, const char *argv[])
{
	struct statfs s1;
	struct statvfs s2;
	const char *path;
	unsigned char buf[sizeof(fsid_t)];
	int i;

	if (argc != 2) {
		printf("Usage: fsid <path>\n");
		exit(1);
	}

	path = argv[1];
	
	if (statfs(path, &s1) != 0) {
		perror("statfs");
		exit(1);
	}

	memcpy(buf, &s1.f_fsid, sizeof(fsid_t));

	printf("statfs  ");
	for (i=0;i<sizeof(buf);i++) {
		printf("%02x", buf[i]);
	}
	printf("\n");

	if (statvfs(path, &s2) != 0) {
		perror("statvfs");
		exit(1);
	}

	printf("statvfs %llx\n", (unsigned long long)s2.f_fsid);

	return 0;	
}
