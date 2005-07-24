/*
  test readdir/unlink pattern that OS/2 uses
  tridge@samba.org July 2005
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define NUM_FILES 29

#define TESTDIR "test.dir"

#define FAILED() (fprintf(stderr, "Failed at %s:%d - %s\n", __FUNCTION__, __LINE__, strerror(errno)), exit(1), 1)

static void cleanup(void)
{
	/* I'm a lazy bastard */
	system("rm -rf " TESTDIR);
	mkdir(TESTDIR, 0700) == 0 || FAILED();
}

static void create_files()
{
	int i;
	for (i=0;i<NUM_FILES;i++) {
		char fname[40];
		snprintf(fname, sizeof(fname), TESTDIR "/test%u.txt", i);
		close(open(fname, O_CREAT|O_RDWR, 0600)) == 0 || FAILED();
	}
}

int main(void)
{
	int total_deleted = 0;
	DIR *d;
	struct dirent *de;

	cleanup();
	create_files();
	
	d = opendir(TESTDIR);

	chdir(TESTDIR) == 0 || FAILED();

	/* skip past . and .. */
	de = readdir(d);
	strcmp(de->d_name, ".") == 0 || FAILED();
	de = readdir(d);
	strcmp(de->d_name, "..") == 0 || FAILED();

	while ((de = readdir(d))) {
		off_t ofs = telldir(d);
		unlink(de->d_name) == 0 || FAILED();

		/* move one more position on */
		readdir(d);

		/* seek to just after the first readdir() */
		seekdir(d, ofs);
		total_deleted++;
	}
	closedir(d);

	printf("Deleted %d files of %d\n", total_deleted, NUM_FILES);

	chdir("..") == 0 || FAILED();

	rmdir(TESTDIR) == 0 || FAILED();

	return 0;
}
