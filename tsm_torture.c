/*
  test program to heavily stress a TSM/HSM system

  Andrew Tridgell January 2008

  compile with:

     gcc -Wall -g -DWITH_GPFS=1 -o tsm_torture{,.c} -lgpfs_gpl -lrt

  If you want to use the -L or -S switches then you must symlink tsm_torture to smbd as 
  otherwise it won't have permission to set share modes or leases

     ln -s tsm_torture smbd

  and run like this:

    ./smbd /gpfs/data/tsmtest

  where /gpfs/data/tsmtest is the directory to test on

 */

#define _XOPEN_SOURCE 500
#define _GNU_SOURCE 

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <utime.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <aio.h>
#if WITH_GPFS
#include "gpfs_gpl.h"
#endif

/* The signal we'll use to signify aio done. */
#ifndef RT_SIGNAL_AIO
#define RT_SIGNAL_AIO (SIGRTMIN+3)
#endif

static struct {
	bool use_lease;
	bool use_sharemode;
	unsigned nprocesses;
	unsigned nfiles;
	unsigned timelimit;
	unsigned fsize;
	const char *dir;
	const char *migrate_cmd;
	bool use_aio;
	uid_t io_uid;
	bool skip_file_creation;
	bool exit_child_on_error;
} options = {
	.use_lease     = false,
	.use_sharemode = false,
	.nprocesses    = 10,
	.nfiles        = 10,
	.fsize         = 8192,
	.timelimit     = 30,
	.migrate_cmd   = "dsmmigrate",
	.use_aio       = false,
	.skip_file_creation = false,
	.exit_child_on_error = false,
};

static pid_t parent_pid;

enum offline_op {OP_LOADFILE, OP_SAVEFILE, OP_MIGRATE, OP_GETOFFLINE, OP_ENDOFLIST};

struct child {
	unsigned child_num;
	unsigned offline_count;
	unsigned online_count;
	unsigned migrate_fail_count;
	unsigned io_fail_count;
	unsigned migrate_ok_count;
	unsigned count;
	unsigned lastcount;
	struct timeval tv_start;
	double latencies[OP_ENDOFLIST];
	double worst_latencies[OP_ENDOFLIST];
	pid_t pid;
	enum offline_op op;
};

static struct timeval tv_start;
static struct child *children;

static unsigned char *buf;

/* return a pointer to a /dev/zero shared memory segment of size "size"
   which will persist across fork() but will disappear when all processes
   exit 

   The memory is zeroed automatically

   This relies on /dev/zero being shared mmap capable, which it is
   only under some OSes (Linux 2.1 _not_ included)
 */
void *shm_setup(int size)
{
	void *ret;
	int fd;

	fd = open("/dev/zero", O_RDWR);
	if (fd == -1) {
		return NULL;
	}
	ret = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	/* note that we don't need to keep the file open */
	close(fd);
	if (ret == (void *)-1) return NULL;
	return ret;
}


static struct timeval timeval_current(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv;
}

static double timeval_elapsed(struct timeval *tv)
{
	struct timeval tv2 = timeval_current();
	return (tv2.tv_sec - tv->tv_sec) + 
	       (tv2.tv_usec - tv->tv_usec)*1.0e-6;
}

/*
  file name given a number
 */
static char *filename(int i)
{
	char *s = NULL;
	asprintf(&s, "%s/file%u.dat", options.dir, (unsigned)i);
	return s;
}

static void sigio_handler(int sig)
{
	printf("Got SIGIO\n");
}

static volatile bool signal_received;

static void signal_handler(int sig)
{
	signal_received = true;
}

/* simulate pread using aio */
static ssize_t pread_aio(int fd, void *buf, size_t count, off_t offset)
{
	struct aiocb acb;
	int ret;

	memset(&acb, 0, sizeof(acb));

	acb.aio_fildes = fd;
	acb.aio_buf = buf;
	acb.aio_nbytes = count;
	acb.aio_offset = offset;
	acb.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	acb.aio_sigevent.sigev_signo  = RT_SIGNAL_AIO;
	acb.aio_sigevent.sigev_value.sival_int = 1;

	signal(RT_SIGNAL_AIO, signal_handler);
	signal_received = 0;

	if (options.io_uid) {
		if (seteuid(options.io_uid) != 0) {
			printf("Failed to become uid %u\n", options.io_uid);
			exit(1);
		}
	}
	if (aio_read(&acb) != 0) {
		return -1;
	}
	if (options.io_uid) {
		if (seteuid(0) != 0) {
			printf("Failed to become root\n");
			exit(1);
		}
	}

	while (signal_received == 0) {
		usleep(500);
	}

	ret = aio_error(&acb);
	if (ret != 0) {
		printf("aio operation failed - %s\n", strerror(ret));
		return -1;
	}

	return aio_return(&acb);	
}


/* simulate pwrite using aio */
static ssize_t pwrite_aio(int fd, void *buf, size_t count, off_t offset)
{
	struct aiocb acb;
	int ret;

	memset(&acb, 0, sizeof(acb));

	acb.aio_fildes = fd;
	acb.aio_buf = buf;
	acb.aio_nbytes = count;
	acb.aio_offset = offset;
	acb.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	acb.aio_sigevent.sigev_signo  = RT_SIGNAL_AIO;
	acb.aio_sigevent.sigev_value.sival_int = 1;

	signal(RT_SIGNAL_AIO, signal_handler);
	signal_received = 0;

	if (options.io_uid) {
		if (seteuid(options.io_uid) != 0) {
			printf("Failed to become uid %u\n", options.io_uid);
			exit(1);
		}
	}
	if (aio_write(&acb) != 0) {
		return -1;
	}
	if (options.io_uid) {
		if (seteuid(0) != 0) {
			printf("Failed to become root\n");
			exit(1);
		}
	}

	while (signal_received == 0) {
		usleep(500);
	}

	ret = aio_error(&acb);
	if (ret != 0) {
		printf("aio operation failed - %s\n", strerror(ret));
		return -1;
	}

	return aio_return(&acb);	
}

/* 
   load a file 
 */
static void child_loadfile(struct child *child, const char *fname, unsigned fnumber)
{
	int fd, i;
	ssize_t ret;

	signal(SIGIO, sigio_handler);

	fd = open(fname, O_RDONLY);
	if (fd == -1) {
		perror(fname);
		if (options.exit_child_on_error) {
			exit(1);
		}
		child->io_fail_count++;
		return;
	}

#if WITH_GPFS
	if (options.use_lease && gpfs_set_lease(fd, GPFS_LEASE_READ) != 0) {
		printf("gpfs_set_lease on '%s' - %s\n", fname, strerror(errno));
		close(fd);
		return;
	}
	if (options.use_sharemode && gpfs_set_share(fd, 1, 2) != 0) {
		printf("gpfs_set_share on '%s' - %s\n", fname, strerror(errno));
		close(fd);
		return;
	}
#endif

	if (options.use_aio) {
		ret = pread_aio(fd, buf, options.fsize, 0);
	} else {
		ret = pread(fd, buf, options.fsize, 0);
	}
	if (ret != options.fsize) {
		printf("pread failed on '%s' - %s\n", fname, strerror(errno));
		child->io_fail_count++;
		close(fd);
		return;
	}

	for (i=0;i<options.fsize;i++) {
		if (buf[i] != 1+(fnumber % 255)) {
			printf("Bad data %u - expected %u for '%s'\n",
			       buf[i], 1+(fnumber%255), fname);
			if (options.exit_child_on_error) {
				exit(1);
			}
			child->io_fail_count++;
			break;
		}
	}

	close(fd);
}


/* 
   save a file 
 */
static void child_savefile(struct child *child, const char *fname, unsigned fnumber)
{
	int fd;
	int ret;

	signal(SIGIO, sigio_handler);

	fd = open(fname, O_WRONLY);
	if (fd == -1) {
		perror(fname);
		exit(1);
	}

#if WITH_GPFS
	if (options.use_lease && gpfs_set_lease(fd, GPFS_LEASE_WRITE) != 0) {
		printf("gpfs_set_lease on '%s' - %s\n", fname, strerror(errno));
		close(fd);
		return;
	}
	if (options.use_sharemode && gpfs_set_share(fd, 1, 2) != 0) {
		printf("gpfs_set_share on '%s' - %s\n", fname, strerror(errno));
		close(fd);
		return;
	}
#endif

	memset(buf, 1+(fnumber%255), options.fsize);

	if (options.use_aio) {
		ret = pwrite_aio(fd, buf, options.fsize, 0);
	} else {
		ret = pwrite(fd, buf, options.fsize, 0);
	}
	if (ret != options.fsize) {
		printf("pwrite failed on '%s' - %s\n", fname, strerror(errno));
		child->io_fail_count++;
		close(fd);
		return;
	}

	fsync(fd);
	close(fd);
}

/* 
   get file offline status
 */
static void child_getoffline(struct child *child, const char *fname)
{
	struct stat st;
	if (stat(fname, &st) != 0) {
		printf("Failed to stat '%s' - %s\n", fname, strerror(errno));
		if (options.exit_child_on_error) {
			exit(1);
		}
		child->io_fail_count++;
		return;
	}
	if (st.st_size != options.fsize) {
		printf("Wrong file size for '%s' - %u\n", fname, (unsigned)st.st_size);
		if (options.exit_child_on_error) {
			exit(1);
		}
		child->io_fail_count++;
		return;
	}
	if (st.st_blocks == 0) {
		child->offline_count++;
		if (strcmp(options.migrate_cmd, "/bin/true") == 0) {
			printf("File '%s' is offline with no migration command\n", fname);
		}
	} else {
		child->online_count++;
	}
}


/* 
   set a file offline
 */
static void child_migrate(struct child *child, const char *fname)
{
	char *cmd = NULL;
	int ret;
	struct utimbuf t;
	struct stat st;

	if (stat(fname, &st) != 0) {
		printf("Failed to stat '%s' - %s\n", fname, strerror(errno));
		if (options.exit_child_on_error) {
			exit(1);
		}
		child->io_fail_count++;
		return;
	}
	if (st.st_size != options.fsize) {
		printf("Wrong file size for '%s' - %u\n", fname, (unsigned)st.st_size);
		if (options.exit_child_on_error) {
			exit(1);
		}
		child->io_fail_count++;
		return;
	}
	if (st.st_blocks == 0) {
		/* already offline */
		return;
	}

	/* make the file a bit older so migation works */
	t.actime = 0;
	t.modtime = time(NULL) - 60*60;
	utime(fname, &t);

	asprintf(&cmd, "%s %s > /dev/null 2>&1", options.migrate_cmd, fname);
	ret = system(cmd);
	if (ret != -1) {
		ret = WEXITSTATUS(ret);
	}
	if (ret != 0) {
		children->migrate_fail_count++;
	} else {
		children->migrate_ok_count++;
	}
	free(cmd);
}


/*
  main child loop
 */
static void run_child(struct child *child)
{
	srandom(time(NULL) ^ getpid());

	while (1) {
		double latency;
		unsigned fnumber = random() % options.nfiles;
		char *fname = filename(fnumber);

		if (kill(parent_pid, 0) != 0) {
			/* parent has exited */
			exit(0);
		}

		child->tv_start = timeval_current();

		child->op = random() % OP_ENDOFLIST;
		switch (child->op) {
		case OP_LOADFILE:
			child_loadfile(child, fname, fnumber);
			break;
		case OP_SAVEFILE:
			child_savefile(child, fname, fnumber);
			break;
		case OP_MIGRATE:
			child_migrate(child, fname);
			break;
		case OP_GETOFFLINE:
			child_getoffline(child, fname);
			break;
		case OP_ENDOFLIST:
			break;
		}

		latency = timeval_elapsed(&child->tv_start);
		if (latency > child->latencies[child->op]) {
			child->latencies[child->op] = latency;
		}
		if (latency > child->worst_latencies[child->op]) {
			child->worst_latencies[child->op] = latency;
		}
		child->count++;

		free(fname);
	}
}

static void sig_alarm(int sig)
{
	int i, op;
	unsigned total=0, total_offline=0, total_online=0, 
		total_migrate_failures=0, total_migrate_ok=0,
		total_io_failures=0;
	double latencies[OP_ENDOFLIST];
	double worst_latencies[OP_ENDOFLIST];
	
	if (timeval_elapsed(&tv_start) >= options.timelimit) {
		printf("timelimit reached - killing children\n");
		for (i=0;i<options.nprocesses;i++) {
			kill(children[i].pid, SIGTERM);
		}
	}

	for (op=0;op<OP_ENDOFLIST;op++) {
		latencies[op] = 0;
		worst_latencies[op] = 0;
	}

	for (i=0;i<options.nprocesses;i++) {
		if (kill(children[i].pid, 0) != 0) {
			continue;
		}
		total += children[i].count - children[i].lastcount;
		children[i].lastcount = children[i].count;		
		total_online += children[i].online_count;
		total_offline += children[i].offline_count;
		total_migrate_failures += children[i].migrate_fail_count;
		total_io_failures += children[i].io_fail_count;
		total_migrate_ok += children[i].migrate_ok_count;
		for (op=0;op<OP_ENDOFLIST;op++) {
			if (children[i].latencies[op] > latencies[op]) {
				latencies[op] = children[i].latencies[op];
			}
			children[i].latencies[op] = 0;
		}
		if (timeval_elapsed(&children[i].tv_start) > latencies[children[i].op]) {
			double lat;
			lat = timeval_elapsed(&children[i].tv_start);
			latencies[children[i].op] = lat;
			if (lat > worst_latencies[children[i].op]) {
				worst_latencies[children[i].op] = lat;
			}
		}
		for (op=0;op<OP_ENDOFLIST;op++) {
			double lat = children[i].worst_latencies[op];
			if (lat > worst_latencies[op]) {
				worst_latencies[op] = lat;
			}
		}
	}

	printf("ops/s=%4u offline=%u/%u  failures: mig=%u io=%u  latencies: mig=%4.1f/%4.1f stat=%4.1f/%4.1f write=%4.1f/%4.1f read=%4.1f/%4.1f\n",
	       total, total_offline, total_online+total_offline, 
	       total_migrate_failures,
	       total_io_failures,
	       latencies[OP_MIGRATE], worst_latencies[OP_MIGRATE],
	       latencies[OP_GETOFFLINE], worst_latencies[OP_GETOFFLINE],
	       latencies[OP_SAVEFILE], worst_latencies[OP_SAVEFILE],
	       latencies[OP_LOADFILE], worst_latencies[OP_LOADFILE]);
	fflush(stdout);
	signal(SIGALRM, sig_alarm);
	alarm(1);
}

static void usage(void)
{
	printf("Usage: (note, must run as 'smbd' to use leases or share modes)\n");
	printf("ln -sf tsm_torture smbd\n");
	printf("./smbd [options] <directory>\n");
	printf("Options:\n");
	printf("  -N <nprocs>  number of child processes\n");
	printf("  -F <nfiles>  number of files\n");
	printf("  -t <time>    runtime (seconds)\n");
	printf("  -s <fsize>   file size (bytes)\n");
	printf("  -M <migrate> set file migrate command\n");
	printf("  -U <uid>     do IO as the specified uid\n");
	printf("  -L           use gpfs leases\n");
	printf("  -S           use gpfs sharemodes\n");
	printf("  -A           use Posix async IO\n");
	printf("  -C           skip file creation\n");
	printf("  -E           exit child on IO error\n");
	exit(0);
}

int main(int argc, char * const argv[])
{
	int opt, i;
	const char *progname = argv[0];
	struct stat st;

	/* parse command-line options */
	while ((opt = getopt(argc, argv, "LSN:F:t:s:M:U:AhCE")) != -1) {
		switch (opt){
		case 'L':
			options.use_lease = true;
			break;
		case 'S':
			options.use_sharemode = true;
			break;
		case 'A':
			options.use_aio = true;
			break;
		case 'C':
			options.skip_file_creation = true;
			break;
		case 'N':
			options.nprocesses = atoi(optarg);
			break;
		case 'F':
			options.nfiles = atoi(optarg);
			break;
		case 'M':
			options.migrate_cmd = strdup(optarg);
			break;
		case 's':
			options.fsize = atoi(optarg);
			break;
		case 'U':
			options.io_uid = atoi(optarg);
			break;
		case 't':
			options.timelimit = atoi(optarg);
			break;
		case 'E':
			options.exit_child_on_error = true;
			break;
		default:
			usage();
			break;
		}
	}

	if ((options.use_lease || options.use_sharemode) && strstr(progname, "smbd") == NULL) {
		printf("ERROR: you must invoke as smbd to use leases or share modes - use a symlink\n");
		exit(1);
	}

	setlinebuf(stdout);	

	argv += optind;
	argc -= optind;

	if (argc == 0) {
		usage();
	}

	options.dir = argv[0];

	if (stat(options.dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
		printf("'%s' must exist and be a directory\n", options.dir);
		exit(1);
	}

	children = shm_setup(sizeof(*children) * options.nprocesses);


	buf = malloc(options.fsize);

	if (!options.skip_file_creation) {
		printf("Creating %u files of size %u in '%s'\n", 
		       options.nfiles, options.fsize, options.dir);

		for (i=0;i<options.nfiles;i++) {
			int fd;
			char *fname = filename(i);
			fd = open(fname, O_CREAT|O_RDWR, 0600);
			if (fd == -1) {
				perror(fname);
				exit(1);
			}
			ftruncate(fd, options.fsize);
			memset(buf, 1+(i%255), options.fsize);
			if (write(fd, buf, options.fsize) != options.fsize) {
				printf("Failed to write '%s'\n", fname);
				exit(1);
			}
			fsync(fd);
			close(fd);
			free(fname);
		}
	}

	parent_pid = getpid();

	printf("Starting %u child processes for %u seconds\n", 
	       options.nprocesses, options.timelimit);
	printf("Results shown as: offline=numoffline/total latencies: current/worst\n");

	for (i=0;i<options.nprocesses;i++) {
		pid_t pid = fork();
		if (pid == 0) {
			children[i].pid = getpid();
			children[i].child_num = i;
			run_child(&children[i]);
		} else {
			children[i].pid = pid;
		}
	}

	/* show status once a second */
	signal(SIGALRM, sig_alarm);
	tv_start = timeval_current();
	alarm(1);

	/* wait for the children to finish */
	for (i=0;i<options.nprocesses;i++) {
		while (waitpid(-1, 0, 0) != 0 && errno != ECHILD) ;
	}	

	return 0;
}
