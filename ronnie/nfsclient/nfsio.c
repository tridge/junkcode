/*
  simulate IO and thread pattern in ntap sio test

  Copyright (C) Andrew Tridgell <tridge@samba.org> 2007

  Released under the GNU GPL version 2 or later
*/

#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE64_SOURCE
#define _LARGE_FILES
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <stdbool.h>
#include "mount.h"
#include "nfs.h"

static size_t block_size = 8192;
static off_t file_size = 1024*1024*(1024LL);
static unsigned num_threads = 1;
static const char *fname;
static volatile unsigned *io_count;
static volatile int run_finished;
static unsigned runtime = 20;
static bool sync_io;
static int flush_blocks;

/* filehandle for the mount */
static fhandle3 *mfh;


static fhandle3 *
get_mount_fh(const char *client, char *mntdir)
{
	CLIENT *clnt;
	dirpath mountdir=mntdir;
	mountres3 *mountres;
	fhandle3 *mfh;
	int i;

	clnt = clnt_create(client, MOUNT_PROGRAM, MOUNT_V3, "tcp");
	if (clnt == NULL) {
		printf("ERROR: failed to connect to MOUNT daemon on %s\n", client);
		exit(10);
	}

	mountres=mountproc3_mnt_3(&mountdir, clnt);
	if (mountres == NULL) {
		printf("ERROR: failed to call the MNT procedure\n");
		exit(10);
	}
	if (mountres->fhs_status != MNT3_OK) {
		printf("ERROR: Server returned error %d when trying to MNT\n",mountres->fhs_status);
		exit(10);
	}

	mfh = malloc(sizeof(fhandle3));
	mfh->fhandle3_len = mountres->mountres3_u.mountinfo.fhandle.fhandle3_len;
	mfh->fhandle3_val = malloc(mountres->mountres3_u.mountinfo.fhandle.fhandle3_len);
	memcpy(mfh->fhandle3_val, 
		mountres->mountres3_u.mountinfo.fhandle.fhandle3_val,
		mountres->mountres3_u.mountinfo.fhandle.fhandle3_len);

	clnt_destroy(clnt);

	printf("mount filehandle : %d ", mfh->fhandle3_len);
	for (i=0;i<mfh->fhandle3_len;i++) {
		printf("%02x", mfh->fhandle3_val[i]);
	}
	printf("\n");

	return mfh;
}

static nfs_fh3 *
lookup_fh(CLIENT *clnt, nfs_fh3 *dir, char *name)
{
	nfs_fh3 *fh;
	LOOKUP3args l3args;
	LOOKUP3res *l3res;
	int i;

	l3args.what.dir.data.data_len = dir->data.data_len;
	l3args.what.dir.data.data_val = dir->data.data_val;
	l3args.what.name = name;
	l3res = nfsproc3_lookup_3(&l3args, clnt);		
	if (l3res == NULL) {
		printf("Failed to lookup file %s\n", name);
		exit(10);
	}
	if (l3res->status != NFS3_OK) {
		printf("lookup returned error %d\n", l3res->status);
		exit(10);
	}

	fh = malloc(sizeof(nfs_fh3));
	fh->data.data_len = l3res->LOOKUP3res_u.resok.object.data.data_len;
	fh->data.data_val = malloc(fh->data.data_len);
	memcpy(fh->data.data_val, 
		l3res->LOOKUP3res_u.resok.object.data.data_val,
		fh->data.data_len);

	printf("file filehandle : %d ", fh->data.data_len);
	for (i=0;i<fh->data.data_len;i++) {
		printf("%02x", fh->data.data_val[i]);
	}
	printf("\n");

	return fh;
}

/* return number of bytes written    or -1 if there was an error */
static int
write_data(CLIENT *clnt, nfs_fh3 *fh, offset3 offset, char *buffer, 
		count3 count, enum stable_how stable)
{
	WRITE3args w3args;
	WRITE3res *w3res;

	w3args.file = *fh;
	w3args.offset = offset;
	w3args.count  = count;
	w3args.stable = stable;
	w3args.data.data_len = count;
	w3args.data.data_val = buffer;
	w3res = nfsproc3_write_3(&w3args, clnt);
	if (w3res == NULL) {
		return -1;
	}
	if (w3res->status != NFS3_OK) {
		return -1;
	}
	return w3res->WRITE3res_u.resok.count;
}

static void
commit_data(CLIENT *clnt, nfs_fh3 *fh)
{
	COMMIT3args c3args;
	COMMIT3res *c3res;

	c3args.file   = *fh;
	c3args.offset = 0;
	c3args.count  = 0;

	c3res = nfsproc3_commit_3(&c3args, clnt);
}

static void run_child(int id)
{
	unsigned num_blocks = file_size / block_size;
	char *buf;
	CLIENT *clnt;
	nfs_fh3 *fh;
	enum stable_how stable = UNSTABLE;

	if (sync_io) {
		stable = FILE_SYNC;
	}

	/* connect to NFS */
	clnt = clnt_create("9.155.61.98", NFS_PROGRAM, NFS_V3, "tcp");
	if (clnt == NULL) {
		printf("ERROR: failed to connect to NFS daemon on %s\n", "9.155.61.98");
		exit(10);
	}


	/* get the file filehandle */
	fh = lookup_fh(clnt, (nfs_fh3 *)mfh, fname);

	buf = malloc(block_size);
	memset(buf, 1, block_size);
	
	srandom(id ^ random());

	while (!run_finished) {
		unsigned offset = random() % num_blocks;
		if (write_data(clnt, fh, offset*(off_t)block_size, buf, block_size, stable) != block_size) {
			printf("write_data failed!\n");
			exit(1);
		}
		io_count[id]++;
		if (flush_blocks && io_count[id] % flush_blocks == 0) {
			commit_data(clnt, fh);
		}
	}

	return;
}

/*
  create a thread with initial function fn(private)
*/
static pthread_t thread_start(void (*fn)(int), int id)
{
	pthread_t thread_id;
	pthread_attr_t thread_attr;
	int rc;
	typedef void *(*thread_fn_t)(void *);
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, 0);
	rc = pthread_create(&thread_id, &thread_attr, (thread_fn_t)fn, (void *)id);
	pthread_attr_destroy(&thread_attr);

	if (rc != 0) {
		fprintf(stderr,"Thread create failed for id %d\n", id);
		exit(1);
	}

	return thread_id;
}

/* wait for a thread to exit */
static int thread_join(pthread_t id)
{
	return pthread_join(id, NULL);
}

static void run_test(void)
{
	int i;
	pthread_t *tids;
	time_t started = 0;
	unsigned warmup_count=0;
	unsigned total;

	/* get the filehandle for the mountpoint */
	mfh = get_mount_fh("9.155.61.98", "/gpfs1/data");

	tids = calloc(num_threads, sizeof(pthread_t));
	io_count = calloc(num_threads, sizeof(unsigned));


	for (i=0;i<num_threads;i++) {
		tids[i] = thread_start(run_child, i);
	}
	
	while (1) {
		int in_warmup=0;
		time_t t = time(NULL);
		total = 0;

		sleep(1);
		if (started == 0) {
			in_warmup = 1;
			started = t;
		}
		for (i=0;i<num_threads;i++) {
			unsigned count = io_count[i];
			printf("%6u ", count);
			if (count == 0) {
				started = 0;
			}
			total += count;
		}
		printf("    total: %6u ", total);
		if (started != 0 && started != t) {
			printf(" %3u seconds  %.0f kbytes/s\n", 
			       (unsigned)(t - started),
			       block_size*(total-warmup_count)/(1024.0*(t-started)));
		} else {
			printf("warmup\n");
		}
		if (started != 0) {
			if (in_warmup) {
				printf("Starting run\n");
				warmup_count = total;
			}
			if (time(NULL) - started >= runtime) {
				break;
			}
		}
	}

	run_finished = 1;
	printf("Run finished - cleaning up\n");
	for (i=0;i<num_threads;i++) {
		thread_join(tids[i]);
		printf("finished thread %d\n", i);
	}

	printf("Throughput %.0f kbytes/s\n", 
	       block_size*(total-warmup_count)/(1024.0*runtime));
}

static void usage(void)
{
	printf("Usage: threadio <options> <filename>\n");
	printf("Options:\n");
	printf(" -b <size>     block size\n");
	printf(" -f <size>     file size\n");
	printf(" -n <number>   number of threads\n");	
	printf(" -t <time>     runtime in seconds\n");
	printf(" -F <numios>   fsync every numios blocks per thread\n");
	printf(" -S            use FILE_SYNC for writes\n");
}


int main(int argc, char * const argv[])
{
	int opt;

	setlinebuf(stdout);
	
	while ((opt = getopt(argc, argv, "b:f:n:t:1F:SD")) != -1) {
		switch (opt) {
		case 'b':
			block_size = strtoul(optarg, NULL, 0);
			break;
		case 'f':
			file_size = strtoull(optarg, NULL, 0);
			break;
		case 'n':
			num_threads = strtoul(optarg, NULL, 0);
			break;
		case 't':
			runtime = strtoul(optarg, NULL, 0);
			break;
		case 'S':
			sync_io = true;
			break;
		case 'F':
			flush_blocks = strtoul(optarg, NULL, 0);
			break;
		default:
			printf("Invalid option '%c'\n", opt);
			usage();
			exit(1);
		}
	}

	argv += optind;
	argc -= optind;

	if (argc < 1) {
		usage();
		exit(1);
	}

	fname = argv[0];

	srandom(time(NULL));

	run_test();
	return 0;
}
