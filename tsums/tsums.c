/* 
   Trivial tripwire-like system

   Copyright (C) Andrew Tridgell 2001
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "tsums.h"

struct sum_struct {
	time_t mtime;
	time_t ctime;
	mode_t mode;
	uid_t uid;
	gid_t gid;
	char sum[16];
};


static TDB_CONTEXT *tdb;
static int do_update;

static void tsums_dir(const char *dname);

static void fatal(const char *format, ...)
{
	va_list ap;
	char *ptr = NULL;

	va_start(ap, format);
	vasprintf(&ptr, format, ap);
	va_end(ap);
	
	if (!ptr || !*ptr) return;

	fprintf(stderr, "%s", ptr);

	free(ptr);
	exit(1);
}


static int dump_fn(TDB_CONTEXT *db, TDB_DATA key, TDB_DATA data, void *state)
{
	if (strncmp(key.dptr, "IGNORE:", 7) == 0) {
		printf("%s\n", key.dptr);
	}
	return 0;
}

static void dump_ignored(void)
{
	tdb_traverse(tdb, dump_fn, NULL);
}


static void report_difference(const char *fname, 
			      struct sum_struct *sum2, 
			      struct sum_struct *sum1)
{
	
	printf("(%c%c%c%c%c%c)\t%s\n", 
	       sum1->ctime==sum2->ctime?' ':'c',
	       sum1->mtime==sum2->mtime?' ':'m',
	       sum1->mode==sum2->mode?' ':'p',
	       sum1->uid==sum2->uid?' ':'u',
	       sum1->gid==sum2->gid?' ':'g',
	       memcmp(sum1->sum, sum2->sum, 16)==0?' ':'4',
	       fname);
}


static int link_checksum(const char *fname, char sum[16])
{
	struct mdfour md;
	char lname[1024];
	int n;

	n = readlink(fname, lname, sizeof(lname));
	if (n == -1) return -1;

	mdfour_begin(&md);
	mdfour_update(&md, lname, n);
	mdfour_result(&md, sum);
	return 0;
}


static int file_checksum(const char *fname, char sum[16])
{
	int fd;
	struct mdfour md;
	unsigned char buf[64*1024];
	
	fd = open(fname,O_RDONLY);
	if (fd == -1) return -1;
	
	mdfour_begin(&md);

	while (1) {
		int n = read(fd, buf, sizeof(buf));
		if (n <= 0) break;
		mdfour_update(&md, buf, n);
	}

	close(fd);

	mdfour_result(&md, sum);
	return 0;
}

static void ignore_file(const char *fname)
{
	TDB_DATA key, data;
	char *keystr=NULL;
	int one=1;

	asprintf(&keystr, "IGNORE:%s", fname);
	key.dptr = keystr;
	key.dsize = strlen(keystr)+1;
	data.dptr = (void *)&one;
	data.dsize = sizeof(one);

	tdb_store(tdb, key, data, TDB_REPLACE);

	printf("Added %s\n", keystr);
	free(keystr);
}

static int is_ignored(const char *fname)
{
	TDB_DATA key, data;
	char *keystr=NULL;

	asprintf(&keystr, "IGNORE:%s", fname);
	key.dptr = keystr;
	key.dsize = strlen(keystr)+1;
	data.dptr = NULL;
	data.dsize = 0;

	data = tdb_fetch(tdb, key);

	free(keystr);

	if (data.dptr) return 1;

	return 0;
}


static void tsums_file(const char *fname)
{
	struct stat st;
	struct sum_struct sum;
	TDB_DATA key, data;
	char *keystr=NULL;

	if (is_ignored(fname)) return;

	if (lstat(fname, &st) != 0) return;

	if (S_ISDIR(st.st_mode)) {
		tsums_dir(fname);
		return;
	}

	bzero(&sum, sizeof(sum));
	sum.mtime = st.st_mtime;
	sum.ctime = st.st_ctime;
	sum.mode = st.st_mode;
	sum.uid = st.st_uid;
	sum.gid = st.st_gid;

	if (S_ISLNK(st.st_mode)) {
		link_checksum(fname, &sum.sum[0]);
	} else {
		file_checksum(fname, &sum.sum[0]);
	}

	asprintf(&keystr, "FILE:%s", fname);

	key.dptr = (void *)keystr;
	key.dsize = strlen(keystr)+1;
	data = tdb_fetch(tdb, key);
	
	if (data.dptr && memcmp(&sum, data.dptr, sizeof(sum)) != 0) {
		report_difference(fname, &sum, (struct sum_struct *)data.dptr);
		if (!do_update) {
			free(keystr);
			return;
		}
	}

	data.dptr = (void *)&sum;
	data.dsize = sizeof(sum);
	tdb_store(tdb, key, data, TDB_REPLACE);
	free(keystr);
}

static void tsums_dir(const char *dname)
{
	DIR *d;
	struct dirent *de;

	d = opendir(dname);
	if (!d) return;

	for (de=readdir(d); de; de=readdir(d)) {
		char *s=NULL;
		if (strcmp(de->d_name, ".") == 0 ||
		    strcmp(de->d_name, "..") == 0) continue;
		asprintf(&s, "%s/%s", dname, de->d_name);
		tsums_file(s);
		free(s);
	}
	closedir(d);
}


static void usage(void)
{
	printf("
Usage: tsums [options] <dirs...>

Options:
  -h        this help
  -u        update sums
  -f        db name
");
	exit(1);
}


int main(int argc, char *argv[])
{
	int i;
	char *db_name = "tsums.tdb";
	extern char *optarg;
	extern int optind;
	int c;
	int do_ignore = 0;
	int do_dump = 0;

	while ((c = getopt(argc, argv, "huf:id")) != -1){
		switch (c) {
		case 'h':
			usage();
			break;
		case 'u':
			do_update = 1;
			break;
		case 'd':
			do_dump = 1;
			break;
		case 'i':
			do_ignore = 1;
			break;
		case 'f':
			db_name = optarg;
			break;
		default:
			usage();
			break;
		}
	}
	

	argc -= optind;
	argv += optind;

	tdb = tdb_open(db_name, 1000, 0, O_CREAT|O_RDWR, 0600);
	
	if (!tdb) {
		fatal("can't open tdb\n");
	}

	if (do_dump) {
		dump_ignored();
		exit(0);
	}

	for (i=0;i<argc;i++) {
		if (do_ignore) {
			ignore_file(argv[i]);
		} else {
			tsums_file(argv[i]);
		}
	}

	tdb_close(tdb);

	return 0;
}
