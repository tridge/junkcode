/*
  simulate IO and thread pattern in ntap sio test

  Copyright (C) Andrew Tridgell <tridge@samba.org> 2007

  Released under the GNU GPL version 2 or later
*/

#define _XOPEN_SOURCE 500
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

static size_t block_size = 8192;
static off_t file_size = 1024*1024*(1024LL);
static unsigned num_threads = 1;
static const char *fname;
static volatile unsigned *io_count;
static volatile int run_finished;
static unsigned runtime = 20;

static void run_child(int id)
{
	int fd;
	unsigned num_blocks = file_size / block_size;
	char *buf;

	buf = malloc(block_size);
	memset(buf, 1, block_size);
	
	fd = open(fname, O_RDWR|O_CREAT, 0644);
	if (fd == -1) {
		perror(fname);
		exit(1);
	}

	srandom(id);

	while (!run_finished) {
		unsigned offset = random() % num_blocks;
		pwrite(fd, buf, block_size, offset*(off_t)block_size);
		io_count[id]++;
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

	for (i=0;i<num_threads;i++) {
		tids[i] = thread_start(run_child, i);
	}
	
	while (1) {
		int in_warmup=0;

		total = 0;

		sleep(1);
		if (started == 0) {
			in_warmup = 1;
			started = time(NULL);
		}
		for (i=0;i<num_threads;i++) {
			unsigned count = io_count[i];
			printf("%6u ", count);
			if (count == 0) {
				started = 0;
			}
			total += count;
		}
		printf("\n");
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
}


int main(int argc, const char *argv[])
{
	int opt;

	setlinebuf(stdout);
	
	while ((opt = getopt(argc, argv, "b:f:n:t:")) != -1) {
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

	run_test();
	return 0;
}
