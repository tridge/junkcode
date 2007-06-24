#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>

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

static __thread int tls;
static int non_tls;

/*
  main thread fn
*/
static void *thread_main(void *private)
{
	tls++;
	non_tls++;
	printf("tls=%d non_tls=%d\n", tls, non_tls);
	return NULL;
}

/* lock a byte range in a open file */
int main(int argc, char *argv[])
{
	thread_start(thread_main, NULL);
	thread_start(thread_main, NULL);
	thread_start(thread_main, NULL);
	thread_start(thread_main, NULL);
	thread_start(thread_main, NULL);

	usleep(1);

	printf("tls=%d non_tls=%d\n", tls, non_tls);

	if (tls != 0 || non_tls != 5) {
		printf("tls failed\n");
		exit(1);
	} else {
		printf("tls OK\n");
	}
	return 0;
}
