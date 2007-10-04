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

static size_t block_size = 8192;
static off_t file_size = 1024*1024*(1024LL);
static unsigned num_threads = 1;
static const char *fname;
static volatile unsigned *io_count;
static volatile int run_finished;
static unsigned runtime = 20;
static bool sync_io;
static bool direct_io;
static int flush_blocks;
static bool one_fd;
static bool reopen;

static int shared_fd;

static int open_file(int id) 
{
	unsigned flags = O_RDWR|O_CREAT|O_LARGEFILE;
	int fd;
	char *name;

	if (sync_io) flags |= O_SYNC;
	if (direct_io) flags |= O_DIRECT;
	if (id != -1 && strchr(fname, '%')) {
		asprintf(&name, fname, id);
	} else {
		name = strdup(fname);
	}
	fd = open(name, flags, 0644);
	if (fd == -1) {
		perror(fname);
		exit(1);
	}
	free(name);
	return fd;
}

static void run_child(int id)
{
	unsigned num_blocks = file_size / block_size;
	char *buf;
	int fd;

	if (!one_fd && !reopen) {
		fd = open_file(id);
	} else {
		fd = shared_fd;
	}

	buf = malloc(block_size);
	memset(buf, 1, block_size);
	
	srandom(id ^ random());

	while (!run_finished) {
		unsigned offset = random() % num_blocks;
		if (reopen) {
			fd = open_file(id);
		}
		if (pwrite(fd, buf, block_size, offset*(off_t)block_size) != block_size) {
			perror("pwrite");
			printf("pwrite failed!\n");
			exit(1);
		}
		io_count[id]++;
		if (flush_blocks && io_count[id] % flush_blocks == 0) {
			fsync(fd);
		}
		if (reopen) {
			close(fd);
		}
	}

	close(fd);
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

	tids = calloc(num_threads, sizeof(pthread_t));
	io_count = calloc(num_threads, sizeof(unsigned));

	if (one_fd) {
		shared_fd = open_file(-1);
	}


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
	printf(" -S            use O_SYNC on open\n");
	printf(" -D            use O_DIRECT on open\n");
	printf(" -R            reopen the file on every write\n");
	printf(" -1            use one file descriptor for all threads\n");
	printf("\nNote: when not using -1, filename may contain %%d for thread id\n");
}


int main(int argc, char * const argv[])
{
	int opt;

	setlinebuf(stdout);
	
	while ((opt = getopt(argc, argv, "b:f:n:t:1F:SDsR")) != -1) {
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
		case 'D':
			direct_io = true;
			break;
		case 'F':
			flush_blocks = strtoul(optarg, NULL, 0);
			break;
		case '1':
			one_fd = true;
			break;
		case 'R':
			reopen = true;
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
