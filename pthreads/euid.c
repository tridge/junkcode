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

/*
  main thread fn
*/
static void *thread_main(void *private)
{
	int tid = *(int *)private;
	uid_t uid, uid2;

	uid = geteuid();
	seteuid(tid);
	uid2 = geteuid();

	printf("tid %d  uid=%d uid2=%d\n", tid, (int)uid, (int)uid2);
	return NULL;
}

/* lock a byte range in a open file */
int main(int argc, char *argv[])
{
	int tid=1;
	uid_t uid;

	thread_start(thread_main, &tid); tid++;
	thread_start(thread_main, &tid); tid++;
	thread_start(thread_main, &tid); tid++;
	thread_start(thread_main, &tid); tid++;
	thread_start(thread_main, &tid); tid++;
	thread_start(thread_main, &tid); tid++;

	uid = geteuid();
	if (uid != 0) {
		printf("euid failed\n");
	}

	return 0;
}
