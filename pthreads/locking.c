#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>

/*
  create a thread with initial function fn(private)
*/
static int thread_start(void *(*fn)(void *private), void *private)
{
	pthread_t thread_id;
	pthread_attr_t thread_attr;
	int rc;

	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	rc = pthread_create(&thread_id, &thread_attr, fn, private);
	pthread_attr_destroy(&thread_attr);
	return rc;
}

/*
  main thread fn
*/
static void *thread_main(void *private)
{
	int tid = (int)private;
	int fd;
	struct flock lock;
	int ret;

	if (tid == 1) {
		fd = open("lock.tst", O_RDWR|O_CREAT, 0644);
	} else {
		fd = open("lock.tst", O_RDWR);
	}
	if (fd == -1) {
		perror("open");
		exit(1);
	}

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 4;
	lock.l_pid = 0;

	ret = fcntl(fd, F_SETLK, &lock);

	printf("tid=%d fd=%d\n", tid, fd);

	if (tid == 1) {
		if (ret != 0) {
			perror("locking/tid1");
			exit(1);
		}
		sleep(2);
	} else {
		if (ret != -1) {
			printf("error: tid2 lock succeeded!\n");
			exit(1);
		}
	}

	return NULL;
}

/* lock a byte range in a open file */
int main(int argc, char *argv[])
{
	int tid=1;

	thread_start(thread_main, tid); tid++;
	sleep(1);
	thread_start(thread_main, tid); tid++;

	return 0;
}
