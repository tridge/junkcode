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
	ino_t inode;
	dev_t device;
	dev_t rdev;
	nlink_t nlink;
	size_t size;
};

struct ignore {
	struct ignore *next;
	char *pattern;
};

static TDB_CONTEXT *tdb;
static int do_update;
static int do_ignore;
static int do_quick;
static int verbose;
static int recurse=1;
static struct ignore *ignore_list;

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


static void dump_ignored(void)
{
	struct ignore *ign;
	for (ign=ignore_list; ign; ign=ign->next) {
		printf("Ignoring %s\n", ign->pattern);
	}
}

static void flush_ignored(void)
{
	struct ignore *ign;
	for (ign=ignore_list; ign; ign=ign->next) {
		char *keystr;
		TDB_DATA key;
		asprintf(&keystr, "IGNORE:%s", ign->pattern);
		key.dptr = keystr;
		key.dsize = strlen(keystr)+1;
		tdb_delete(tdb, key);
		free(keystr);
	}
}


static void report_difference(const char *fname, 
			      struct sum_struct *sum2, 
			      struct sum_struct *sum1)
{
	
	printf("(%c%c%c%c%c%c%c%c%c%c%c)\t%s\n", 
	       sum1->ctime==sum2->ctime?' ':'c',
	       sum1->mtime==sum2->mtime?' ':'m',
	       sum1->mode==sum2->mode?' ':'p',
	       sum1->uid==sum2->uid?' ':'u',
	       sum1->gid==sum2->gid?' ':'g',
	       sum1->inode==sum2->inode?' ':'i',
	       sum1->device==sum2->device?' ':'d',
	       sum1->rdev==sum2->rdev?' ':'r',
	       sum1->nlink==sum2->nlink?' ':'l',
	       sum1->size==sum2->size?' ':'s',
	       do_quick || memcmp(sum1->sum, sum2->sum, 16)==0?' ':'4',
	       fname);
	if (verbose && !do_quick && memcmp(sum1->sum, sum2->sum, 16)) {
		int i;
		printf("\t");
		for (i=0;i<16;i++) printf("%02x", sum1->sum[i]);
		printf("\t");
		for (i=0;i<16;i++) printf("%02x", sum2->sum[i]);
		printf("\n");
	}
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
	struct ignore *ign;
	for (ign=ignore_list; ign; ign=ign->next) {
		if (fnmatch(ign->pattern, fname, 0) == 0) return 1;
	}
	return 0;
}


static void tsums_file(const char *fname)
{
	struct stat st;
	struct sum_struct sum;
	struct sum_struct old;
	TDB_DATA key, data;
	char *keystr=NULL;

	if (lstat(fname, &st) != 0) return;

	if (is_ignored(fname)) goto next;

	bzero(&sum, sizeof(sum));
	sum.mtime = st.st_mtime;
	sum.ctime = st.st_ctime;
	sum.mode = st.st_mode;
	sum.uid = st.st_uid;
	sum.gid = st.st_gid;
	sum.device = st.st_dev;
	sum.inode = st.st_ino;
	sum.nlink = st.st_nlink;
	sum.size = st.st_size;
	if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode)) {
		sum.rdev = st.st_rdev;
	}

	if (S_ISLNK(st.st_mode)) {
		link_checksum(fname, &sum.sum[0]);
	} else if (S_ISREG(st.st_mode) && !do_quick) {
		file_checksum(fname, &sum.sum[0]);
	}

	asprintf(&keystr, "FILE:%s", fname);

	key.dptr = (void *)keystr;
	key.dsize = strlen(keystr)+1;
	data = tdb_fetch(tdb, key);

	if (!data.dptr) goto update;
	
	if (data.dsize==sizeof(sum) &&
	    memcmp(&sum, data.dptr, sizeof(sum)) == 0) {
		/* nothings changed */
		free(data.dptr);
		goto next;
	}

	if (memcmp(&sum, data.dptr, MIN(data.dsize,sizeof(sum))) == 0) {
		/* data structure extended */
		if (verbose) printf("old record size (%d/%d) - updating\n",
				    data.dsize, sizeof(sum));
		free(data.dptr);
		if (do_update) {
			goto update;
		} else {
			goto next;
		}
	}
	
	bzero(&old, sizeof(old));
	memcpy(&old, data.dptr, MIN(sizeof(old), data.dsize));
	free(data.dptr);

	if (do_quick &&
	    old.ctime == sum.ctime &&
	    old.mtime == sum.mtime &&
	    old.uid == sum.uid &&
	    old.gid == sum.gid &&
	    old.mode == sum.mode &&
	    old.device == sum.device &&
	    old.inode == sum.inode &&
	    old.rdev == sum.rdev &&
	    old.nlink == sum.nlink &&
	    old.size == sum.size) {
		/* quick properties are the same */
		goto next;
	}

	report_difference(fname, &sum, &old);

	if (!do_update) {
		goto next;
	}

update:
	if (do_quick && S_ISREG(st.st_mode)) {
		file_checksum(fname, &sum.sum[0]);
	}
	data.dptr = (void *)&sum;
	data.dsize = sizeof(sum);
	tdb_store(tdb, key, data, TDB_REPLACE);

next:
	if (keystr) free(keystr);

	if (recurse && S_ISDIR(st.st_mode)) {
		tsums_dir(fname);
	}
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


static int load_ignore(TDB_CONTEXT *db, TDB_DATA key, TDB_DATA data, 
			void *state)
{
	struct ignore *ignore;

	if (strncmp(key.dptr, "IGNORE:", 7) != 0) return 0;

	ignore = (struct ignore *)malloc(sizeof(*ignore));
	if (!ignore) fatal("out of memory in load_ignore\n");
	ignore->pattern = strdup(key.dptr+7);
	ignore->next = ignore_list;
	ignore_list = ignore;
	return 0;
}

static void process_one(char *fname)
{
	if (verbose>1) {
		printf("%s\n", fname);
	}
	if (do_ignore) {
		ignore_file(fname);
	} else {
		tsums_file(fname);
	}
}

static int process_fn(TDB_CONTEXT *db, TDB_DATA key, TDB_DATA data, 
		      void *state)
{
	if (strncmp(key.dptr, "FILE:", 5) != 0) return 0;

	process_one(key.dptr + 5);
	return 0;
}

static void usage(void)
{
	printf("
tsums maintains signatures of files on a system. Similar to tripwire.
Copyright (C) Andrew Tridgell (tridge@samba.org)

Usage: tsums [options] <files|dirs...>

Options:
  -a          use all existing files
  -q          quick mode (don't checksum)
  -h          this help
  -u          update sums
  -f <DB>     database name
  -i          add listed files to ignore list
  -d          dump the ignored list
  -F          flush the ignored list
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
	int do_dump = 0;
	int use_all = 0;
	int do_flush_ignore=0;

	while ((c = getopt(argc, argv, "qhuf:idavF")) != -1){
		switch (c) {
		case 'h':
			usage();
			break;
		case 'a':
			use_all = 1;
			break;
		case 'v':
			verbose++;
			break;
		case 'q':
			do_quick = 1;
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
		case 'F':
			do_flush_ignore = 1;
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

	tdb_traverse(tdb, load_ignore, NULL);

	if (do_dump) {
		dump_ignored();
		goto finish;
	}

	if (do_flush_ignore) {
		flush_ignored();
		goto finish;
	}

	if (use_all) {
		recurse = 0;
		tdb_traverse(tdb, process_fn, NULL);
		goto finish;
	} 

	if (argc == 0) usage();

	for (i=0;i<argc;i++) {
		process_one(argv[i]);
	}

finish:
	tdb_close(tdb);

	return 0;
}
