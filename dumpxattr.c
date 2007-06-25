#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <attr/xattr.h>


int main(int argc, const char *argv[])
{
	int rc;
	const char *attrname, *fname;
	unsigned char *buf;

	if (argc < 3) {
		printf("usage: dumpxattr <fname> <attrname>\n");
		exit(1);
	}

	fname = argv[1];
	attrname = argv[2];

	rc = getxattr(fname, attrname, NULL, 0);
	if (rc == -1) {
		perror(fname);
		exit(1);
	}

	buf = malloc(rc);

	rc = getxattr(fname, attrname, buf, rc);
	if (rc == -1) {
		perror(fname);
		exit(1);
	}

	write(1, buf, rc);

	return 0;
}
