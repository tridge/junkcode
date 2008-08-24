/*
  test program to demonstrate race between AIO and setresuid()

  tridge@samba.org August 2008

  The race condition is in setresuid(), which in glibc tries to be
  smart about threads and change the euid of threads when the euid of
  the main program changes. The problem is that this makes setresuid()
  non-atomic, which means that if an IO completes during the complex
  series of system calls that setresuid() becomes, then the thread
  completing the IO may get -1/EPERM back from the rt_sigqueueinfo()
  call that it uses to notify its parent of the completing IO. In that
  case two things happen:

    1) the signal is never delivered, so the caller never is told that
    the IO has completed

    2) if the caller polls for completion using aio_error() then it
    will see a -1/EPERM result, rather than the real result of the IO

  The simplest fix in existing code that mixes uid changing with AIO
  (such as Samba) is to not use setresuid() and use setreuid()
  instead, which in glibc doesn't try to play any games with the euid
  of threads. That does mean that you will need to manually gain root
  privileges before calling aio_read() or aio_write() to ensure that
  the thread has permission to send signals to the main thread
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
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <aio.h>

/* The signal we'll use to signify aio done. */
#ifndef RT_SIGNAL_AIO
#define RT_SIGNAL_AIO (SIGRTMIN+3)
#endif

static volatile bool signal_received;

static void signal_handler(int sig)
{
	signal_received = true;
}

#define UID 1000

static void become_root(void)
{
#if USE_SETEUID
	seteuid(0);
#elif USE_SETREUID
	setreuid(-1, 0);
#else
	setresuid(0, 0, -1);
#endif
	if (geteuid() != 0) {
		printf("become root failed\n");
		exit(1);
	}
}

static void unbecome_root(void)
{
#if USE_SETEUID
	seteuid(UID);
#elif USE_SETREUID
	setreuid(-1, UID);
#else
	setresuid(UID, UID, -1);
#endif
	if (geteuid() != UID) {
		printf("become root failed\n");
		exit(1);
	}
}

/* pread using aio */
static ssize_t pread_aio(int fd, void *buf, size_t count, off_t offset)
{
	struct aiocb acb;
	int ret;
	time_t t;

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

	become_root();
	if (aio_read(&acb) != 0) {
		return -1;
	}
	unbecome_root();

	t = time(NULL);
	while (!signal_received) {
		usleep(1000);
		if (time(NULL) - t > 5) {
			printf("Timed out waiting for IO (AIO race)\n");
			exit(1);
		}
	}

	ret = aio_error(&acb);
	if (ret != 0) {
		printf("aio operation failed - %s\n", strerror(ret));
		return -1;
	}

	return aio_return(&acb);	
}

static int count;

static void sig_alarm(int sig)
{
	printf("%6d\r", count);
	count=0;
	fflush(stdout);
	signal(SIGALRM, sig_alarm);
	alarm(1);
}

int main(int argc, const char *argv[])
{
	int fd;
	const char *fname;
	char buf[8192];

	fname = argv[1];

	unbecome_root();

	fd = open(fname, O_RDWR|O_CREAT|O_TRUNC, 0600);

	memset(buf, 1, sizeof(buf));
	write(fd, buf, sizeof(buf));
	fsync(fd);

	signal(SIGALRM, sig_alarm);
	alarm(1);

	while (1) {
		if (pread_aio(fd, buf, sizeof(buf), 0) != sizeof(buf)) {
			break;
		}
		count++;
	}

	return 0;
}
