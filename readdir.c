/*
  a replacement for opendir/readdir/telldir/seekdir/closedir
  
*/

#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>

#define DIR_BUF_BITS 9
#define DIR_BUF_SIZE (1<<DIR_BUF_BITS)

#define O_DIRECTORY 0200000

struct dir_buf {
	int fd;
	int nbytes, ofs;
	off_t seekpos;
	char buf[DIR_BUF_SIZE];
};

_syscall3(int, getdents, uint, fd, struct dirent *, dirp, uint, count)

struct dir_buf *rep_opendir(const char *dname)
{
	struct dir_buf *d;
	d = malloc(sizeof(*d));
	d->fd = open(dname, O_DIRECTORY | O_RDONLY);
	if (d->fd == -1) {
		return NULL;
	}
	d->ofs = 0;
	d->seekpos = 0;
	d->nbytes = 0;
	return d;
}

struct dirent *rep_readdir(struct dir_buf *d)
{
	struct dirent *de;

	if (d->ofs >= d->nbytes) {
		d->seekpos = lseek(d->fd, 0, SEEK_CUR);
		d->nbytes = getdents(d->fd, (struct dirent *)d->buf, DIR_BUF_SIZE);
		d->ofs = 0;
	}
	if (d->ofs >= d->nbytes) {
		return NULL;
	}
	de = (struct dirent *)&d->buf[d->ofs];
	d->ofs += de->d_reclen;
	if (d->ofs >= d->nbytes) {
		d->seekpos = lseek(d->fd, 0, SEEK_CUR);
		d->nbytes = getdents(d->fd, (struct dirent *)d->buf, DIR_BUF_SIZE);
		d->ofs = 0;
	}
	return de;
}

off_t rep_telldir(struct dir_buf *d)
{
	if (d->ofs >= d->nbytes) {
		d->seekpos = lseek(d->fd, 0, SEEK_CUR);
		d->ofs = 0;
		d->nbytes = 0;
	}
	return (d->seekpos << DIR_BUF_BITS) + d->ofs;
}

void rep_seekdir(struct dir_buf *d, off_t ofs)
{
	d->seekpos = lseek(d->fd, ofs >> DIR_BUF_BITS, SEEK_SET);
	d->nbytes = getdents(d->fd, (struct dirent *)d->buf, DIR_BUF_SIZE);
	d->ofs = 0;
	while (d->ofs < (ofs & (DIR_BUF_SIZE-1))) {
		if (rep_readdir(d) == NULL) break;
	}
}

int rep_closedir(struct dir_buf *d)
{
	int r = close(d->fd);
	if (r != 0) {
		return r;
	}
	free(d);
	return 0;
}




/*
  test readdir/unlink pattern that OS/2 uses
  tridge@samba.org July 2005
*/

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
		snprintf(fname, sizeof(fname), TESTDIR "/test%04u.txt", i);
		close(open(fname, O_CREAT|O_RDWR, 0600)) == 0 || FAILED();
	}
}

int main(void)
{
	int total_deleted = 0;
	struct dir_buf *d;
	struct dirent *de;

	cleanup();
	create_files();
	
	d = rep_opendir(TESTDIR);

	chdir(TESTDIR) == 0 || FAILED();

	/* skip past . and .. */
	de = rep_readdir(d);
	strcmp(de->d_name, ".") == 0 || FAILED();
	de = rep_readdir(d);
	strcmp(de->d_name, "..") == 0 || FAILED();

	while ((de = rep_readdir(d))) {
		off_t ofs = rep_telldir(d);
		printf("Deleting %s at %d\n", de->d_name, (int)ofs);
		unlink(de->d_name) == 0 || FAILED();

		/* move one more position on */
		rep_readdir(d);

		/* seek to just after the first readdir() */
		rep_seekdir(d, ofs);
		total_deleted++;
	}
	rep_closedir(d);

	printf("Deleted %d files of %d\n", total_deleted, NUM_FILES);

	chdir("..") == 0 || FAILED();

	rmdir(TESTDIR) == 0 || FAILED();

	return 0;
}
