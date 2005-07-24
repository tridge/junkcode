/*
  a replacement for opendir/readdir/telldir/seekdir/closedir for BSD systems
  
*/

#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>

#define DIR_BUF_BITS 9
#define DIR_BUF_SIZE (1<<DIR_BUF_BITS)

struct dir_buf {
	int fd;
	int nbytes, ofs;
	off_t seekpos;
	char buf[DIR_BUF_SIZE];
};

struct dir_buf *rep_opendir(const char *dname)
{
	struct dir_buf *d;
	d = malloc(sizeof(*d));
	if (d == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	d->fd = open(dname, O_RDONLY);
	if (d->fd == -1) {
		free(d);
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
		d->nbytes = getdents(d->fd, d->buf, DIR_BUF_SIZE);
		d->ofs = 0;
	}
	if (d->ofs >= d->nbytes) {
		return NULL;
	}
	de = (struct dirent *)&d->buf[d->ofs];
	d->ofs += de->d_reclen;
	return de;
}

off_t rep_telldir(struct dir_buf *d)
{
	if (d->ofs >= d->nbytes) {
		d->seekpos = lseek(d->fd, 0, SEEK_CUR);
		d->ofs = 0;
		d->nbytes = 0;
	}
	/* this relies on seekpos always being a multiple of
	   DIR_BUF_SIZE. Is that always true on BSD systems? */
	if (d->seekpos & (DIR_BUF_SIZE-1)) {
		abort();
	}
	return d->seekpos + d->ofs;
}

void rep_seekdir(struct dir_buf *d, off_t ofs)
{
	d->seekpos = lseek(d->fd, ofs & ~(DIR_BUF_SIZE-1), SEEK_SET);
	d->nbytes = getdents(d->fd, d->buf, DIR_BUF_SIZE);
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

