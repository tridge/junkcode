#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>

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
	static char stack[8192];
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

//	printf("args=%p a2=%p\n", args, a2);
//	fflush(stdout);

	ret = args->ret;
	if (ret == -1) {
		errno = args->error;
	}
	free(args);

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
		close(fd);
//		unlink(fname);
		count++;
	}
	printf("%.1f ops/sec (%s)\n", count/end_timer(), name);

	close(aio_pipe[0]);
	close(aio_pipe[1]);
}

int main(void)
{
	run_test("aio_open", aio_open);
	run_test("open", open);
	run_test("aio_open", aio_open);
	run_test("open", open);

	return 0;
}
