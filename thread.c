#define _REENTRANT 
#include <pthread.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#define MAX_THREADS 1024
#define RSIZE (16*1024)
static int num_threads;
static size_t read_size;
static pthread_t tid[MAX_THREADS]; 
static pid_t pid[MAX_THREADS]; 

static void thread_main(void)
{
	char buf[RSIZE];
	int fd1, fd2;
	int i;

	sleep(1);

	fd1 = open("/dev/zero", O_RDONLY);
	fd2 = open("/dev/null", O_WRONLY);
	
	for (i=0;i<read_size;i++) {
		if (read(fd1, buf, RSIZE) != RSIZE) {
			printf("read failed\n");
			exit(1);
		}
		if (write(fd2, buf, RSIZE) != RSIZE) {
			printf("write failed\n");
			exit(1);
		}
	}
	close(fd1);
	close(fd2);
}


static void fork_test(void)
{
	int i;
	time_t start = time(NULL);
	
	for (i=0;i < num_threads; i++) {
		if ((pid[i] = fork()) == 0) {
			thread_main();
			_exit(0);
		}
		if (pid[i] == (pid_t)-1) {
			printf("fork %d failed\n", i);
			exit(1);
		}
	}
	
	for (i=0;i < num_threads; i++) {
		waitpid(pid[i], NULL, 0);
	}  
	
	printf("using fork took %d seconds\n", time(NULL) - start);
}


static void pthread_test(void)
{
	int i;
	time_t start = time(NULL);
	
	for (i=0;i < num_threads; i++) {
		if (pthread_create(&tid[i], NULL, thread_main, NULL) != 0) {
			printf("pthread_create failed\n");
			exit(1);
		}
	}
	
	for (i=0;i < num_threads; i++) {
		pthread_join(tid[i], NULL);
	}
	
	printf("using pthreads took %d seconds\n", time(NULL) - start);
}


int main(int argc, char *argv[]) 
{
	if (argc < 2) {
		printf("usage: %s <num_threads> <num_loops>\n",argv[0]);
		exit(1);
	}
	
	num_threads = atoi(argv[1]);
	read_size = atoi(argv[2]);
	
	if (num_threads > MAX_THREADS) {
		printf("max_threads is %d\n", MAX_THREADS);
		exit(1);
	}
	
	fork_test();
	pthread_test();
	fork_test();
	pthread_test();
	
	printf("all done\n");
}  

