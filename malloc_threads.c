#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

#if USE_THREADS
#include <pthread.h>

/*
  create a thread with initial function fn(private)
*/
static int proc_start(void *(*fn)(void *private))
{
	pthread_t thread_id;
	pthread_attr_t thread_attr;
	int rc;

	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, 0);
	rc = pthread_create(&thread_id, &thread_attr, fn, NULL);
	pthread_attr_destroy(&thread_attr);

	return (int)thread_id;
}

/* wait for a thread to exit */
static int proc_join(int id)
{
	return pthread_join(id, NULL);
}


#else

/*
  create a thread with initial function fn(private)
*/
static int proc_start(void *(*fn)(void *private))
{
	pid_t pid;

	pid = fork();
	if (pid == 0) {
		fn(NULL);
		exit(0);
	}
	return pid;
}

static int proc_join(int id)
{
	if (waitpid((pid_t)id, NULL, 0) != id) {
		return -1;
	}
	return 0;
}

#endif

/*
  main child process fn
*/
static void *proc_main(void *private)
{
#define NMALLOCS 3000
	int i, j;
	void *ptrs[NMALLOCS];
	for (j=0;j<100;j++) {
		for (i=1;i<NMALLOCS;i++) {
			ptrs[i] = malloc(i);
			if (!ptrs[i]) {
				printf("malloc(%d) failed!\n", i);
				exit(1);
			}
		}
		for (i=1;i<NMALLOCS;i++) {
			free(ptrs[i]);
		}
	}
	return NULL;
}

/* lock a byte range in a open file */
int main(int argc, char *argv[])
{
	int nthreads, i;
	int *ids;

	if (argc <= 1) {
		printf("malloc NPROCS\n");
		exit(1);
	}

	nthreads = atoi(argv[1]);

	ids = malloc(sizeof(*ids) * nthreads);

	for (i=0;i<nthreads;i++) {
		ids[i] = proc_start(proc_main);
	}

	for (i=0;i<nthreads;i++) {
		int rc;
		rc = proc_join(ids[i]);
		if (rc != 0) {
			printf("Thread %d failed : %s\n", i, strerror(errno));
			exit(1);
		}
	}

	return 0;
}
