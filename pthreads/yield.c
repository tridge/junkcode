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

static int xx;
const int nthreads = 2;

/*
  main thread fn
*/
static void *thread_main(void *private)
{
	int n = (int)private;

	while (1) {
		if (xx % nthreads == n) {
			n++;
		}
		pthread_yield();
	}

	return NULL;
}

/* lock a byte range in a open file */
int main(int argc, char *argv[])
{
	int i;

	for (i=0;i<nthreads;i++) {
		thread_start(thread_main, i);
	}
	sleep(10);
	return 0;
}
