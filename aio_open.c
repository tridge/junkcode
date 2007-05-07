#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/time.h>

static struct timeval tp1,tp2;

static void start_timer()
{
	gettimeofday(&tp1,NULL);
}

static double end_timer()
{
	gettimeofday(&tp2,NULL);
	return (tp2.tv_sec + (tp2.tv_usec*1.0e-6)) - 
		(tp1.tv_sec + (tp1.tv_usec*1.0e-6));
}

struct aio_open_args {
	const char *fname;
	int flags;
	mode_t mode;
	int error;
	int ret;
};

static int aio_pipe[2];

static int aio_open_child(void *ptr)
{
	struct aio_open_args *args = ptr;
	args->ret = open(args->fname, args->flags, args->mode);
	args->error = errno;
	write(aio_pipe[1], &ptr, sizeof(ptr));
	return 0;
}

static int aio_open(const char *fname, int flags, mode_t mode)
{
	char stack[512];
	struct aio_open_args *args, *a2;
	pid_t pid;
	int ret, status;

	args = malloc(sizeof(*args));
	args->fname = fname;
	args->flags = flags;
	args->mode  = mode;

	pid = clone(aio_open_child, stack+sizeof(stack)-32, 
		    CLONE_FS|CLONE_FILES|CLONE_VM, 
		    args);
	
	if (read(aio_pipe[0], &a2, sizeof(a2)) != sizeof(a2)) {
		errno = EIO;
		return -1;
	}

	waitpid(pid, &status, __WCLONE);

	ret = args->ret;
	if (ret == -1) {
		errno = args->error;
	}
	free(args);

	return ret;
}

enum aio_clone_op {AIO_CLONE_NOOP, AIO_CLONE_OPEN};

struct aio_clone_common {
	char stack[2048];
	enum aio_clone_op op;
	int fd1[2];
	int fd2[2];
	int error;
	int ret;
	pid_t pid;
};

union aio_clone {
	struct {
		struct aio_clone_common common;
	} _noop;
	struct {
		struct aio_clone_common common;
		const char *fname;
		int flags;
		mode_t mode;
	} _open;
};

static int aio_clone_wake(union aio_clone *args)
{
	if (write(args->_open.common.fd1[1], &args, sizeof(args)) != sizeof(args)) {
		return -1;
	}
	return 0;
}

static int aio_clone_finished(union aio_clone *args)
{
	if (write(args->_open.common.fd2[1], &args, sizeof(args)) != sizeof(args)) {
		return -1;
	}
	return 0;
}

static int aio_clone_wait_child(union aio_clone *args)
{
	if (read(args->_open.common.fd1[0], &args, sizeof(args)) != sizeof(args)) {
		return -1;
	}
	return 0;
}

static int aio_clone_wait_parent(union aio_clone *args)
{
	if (read(args->_open.common.fd2[0], &args, sizeof(args)) != sizeof(args)) {
		return -1;
	}
	return 0;
}

static int aio_clone_child(void *ptr)
{
	union aio_clone *args = ptr;
	while (aio_clone_wait_child(args) == 0) {
		struct aio_clone_common *common = &args->_noop.common;
		switch (common->op) {
		case AIO_CLONE_NOOP:
			common->ret = 0;
			break;
		case AIO_CLONE_OPEN:
			common->ret = open(args->_open.fname, 
					   args->_open.flags, 
					   args->_open.mode);
			break;
		}
		if (common->ret == -1) {
			common->error = errno;
		}
		if (aio_clone_finished(args) != 0) break;
	}
	return -1;
}

static union aio_clone *aio_clone_start(void)
{
	union aio_clone *args = calloc(1, sizeof(*args));
	pipe(args->_noop.common.fd1);
	pipe(args->_noop.common.fd2);
	args->_noop.common.pid = 
		clone(aio_clone_child, args->_noop.common.stack+
		      sizeof(args->_noop.common.stack)-32, 
		      CLONE_FS|CLONE_FILES|CLONE_VM, 
		      (void*)args);
	sleep(1);
	return args;
}


static int aio_open2(const char *fname, int flags, mode_t mode)
{
	static union aio_clone *args;
	int ret;

	if (args == NULL) {
		args = aio_clone_start();
	}

	args->_open.common.op    = AIO_CLONE_OPEN;
	args->_open.fname = fname;
	args->_open.flags = flags;
	args->_open.mode  = mode;

	if (aio_clone_wake(args) != 0) {
		errno = EIO;
		return -1;
	}

	if (aio_clone_wait_parent(args) != 0) {
		errno = EIO;
		return -1;
	}

	ret = args->_open.common.ret;
	if (ret == -1) {
		errno = args->_open.common.error;
	}

	return ret;
}


static int aio_noop(const char *fname, int flags, mode_t mode)
{
	static union aio_clone *args;
	int ret;

	if (args == NULL) {
		args = aio_clone_start();
	}

	args->_noop.common.op = AIO_CLONE_NOOP;

	if (aio_clone_wake(args) != 0) {
		errno = EIO;
		return -1;
	}
	
	if (aio_clone_wait_parent(args) != 0) {
		errno = EIO;
		return -1;
	}

	ret = args->_open.common.ret;
	if (ret == -1) {
		errno = args->_open.common.error;
	}

	return ret;
}


static void run_test(const char *name, int (*fn)(const char *, int, mode_t))
{
	const char *fname = "test.dat";
	const int timeout=3;
	int count=0;

	pipe(aio_pipe);

	start_timer();
	while (end_timer() < timeout) {
		int fd = fn(fname, O_CREAT|O_TRUNC|O_RDWR, 0600);
		if (fd == -1) {
			perror(fname);
			exit(1);
		}
		if (fd != 0) close(fd);
//		unlink(fname);
		count++;
	}
	printf("%.1f usec/op (%s)\n", 1.0e6*end_timer()/count, name);

	close(aio_pipe[0]);
	close(aio_pipe[1]);
}

int main(void)
{
	run_test("aio_open2", aio_open2);

	run_test("noop", aio_noop);

	run_test("aio_open", aio_open);
	run_test("open", open);
	run_test("aio_open2", aio_open2);

	run_test("aio_open", aio_open);
	run_test("open", open);
	run_test("aio_open2", aio_open2);

	return 0;
}
