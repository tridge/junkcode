#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>

/*
  create a thread with initial function fn(private)
*/
static pthread_t thread_start(void *(*fn)(int), int id)
{
	pthread_t thread_id;
	pthread_attr_t thread_attr;
	int rc;
	typedef void *(*thread_fn_t)(void *);
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, 0);
	rc = pthread_create(&thread_id, &thread_attr, (thread_fn_t)fn, (void *)id);
	pthread_attr_destroy(&thread_attr);

	return thread_id;
}

/* wait for a thread to exit */
static int thread_join(pthread_t id)
{
	return pthread_join(id, NULL);
}


/*
  create a process with initial function fn(private)
*/
static pid_t proc_start(void *(*fn)(int), int id)
{
	pid_t pid;

	pid = fork();
	if (pid == 0) {
		fn(id);
		exit(0);
	}
	return pid;
}

/* wait for a process to exit */
static int proc_join(pid_t id)
{
	if (waitpid(id, NULL, 0) != id) {
		return -1;
	}
	return 0;
}


/* run a function under a set of threads */
static void run_threads(int nthreads, void *(*fn)(int ))
{
	int i;
	pthread_t *ids = malloc(sizeof(*ids) * nthreads);

	for (i=0;i<nthreads;i++) {
		ids[i] = thread_start(fn, i);
	}

	for (i=0;i<nthreads;i++) {
		int rc;
		rc = thread_join(ids[i]);
		if (rc != 0) {
			fprintf(stderr, "Thread %d failed : %s\n", i, strerror(errno));
			exit(1);
		}
	}
}

/* run a function under a set of processes */
static void run_processes(int nprocs, void *(*fn)(int ))
{
	int i;
	pid_t *ids = malloc(sizeof(*ids) * nprocs);

	for (i=0;i<nprocs;i++) {
		ids[i] = proc_start(fn, i);
	}

	for (i=0;i<nprocs;i++) {
		int rc;
		rc = proc_join(ids[i]);
		if (rc != 0) {
			fprintf(stderr, "Process %d failed : %s\n", i, strerror(errno));
			exit(1);
		}
	}
}



/***********************************************************************
  a simple malloc speed test using a wide variety of malloc sizes
************************************************************************/
static void *test_malloc(int id)
{
#define NMALLOCS 300
	int i, j;
	void *ptrs[NMALLOCS];
	for (j=0;j<500;j++) {
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


/***********************************************************************
 simple read/write testing using /dev/null and /dev/zero
************************************************************************/
static void *test_readwrite(int id)
{
	int i;
	int fd_in, fd_out;
	/* we use less than 1 page to prevent page table games */
	char buf[4095];
	fd_in = open("/dev/zero", O_RDONLY);
	fd_out = open("/dev/null", O_WRONLY);
	if (fd_in == -1 || fd_out == -1) {
		fprintf(stderr,"Failed to open /dev/zero or /dev/null\n");
		exit(1);
	}

	for (i=0;i<20000;i++) {
		if (read(fd_in, buf, sizeof(buf)) != sizeof(buf) ||
		    write(fd_out, buf, sizeof(buf)) != sizeof(buf)) {
			fprintf(stderr,"IO failed at loop %d\n", i);
			exit(1);
		}
	}

	close(fd_in);
	close(fd_out);
	
	return NULL;
}


/***********************************************************************
test stat() operations
************************************************************************/
static void *test_stat(int id)
{
	int i;
	char fname[60];
        char dname[30];

        sprintf(dname, "testd_%d", id);
        rmdir(dname);

        if (mkdir(dname, 0777) != 0) goto failed;

	sprintf(fname, "%s/test%d.dat", dname, id);

	for (i=0;i<10000;i++) {
		struct stat st;
		if (stat(dname, &st) != 0) goto failed;
		if (stat(fname, &st) == 0) goto failed;
	}

        if (rmdir(dname) != 0) goto failed;
	
	return NULL;

failed:
	fprintf(stderr,"stat failed\n");
	exit(1);
}

/***********************************************************************
test directory operations
************************************************************************/
static void *test_dir(int id)
{
        int i;
        char dname[30];

        sprintf(dname, "testd_%d", id);
        rmdir(dname);

        if (mkdir(dname, 0777) != 0) goto failed;

        for (i=0;i<2000;i++) {
                DIR *d = opendir(dname);
                if (!d) goto failed;
                while (readdir(d)) {} ;
                closedir(d);
        }

        if (rmdir(dname) != 0) goto failed;

        return NULL;

failed: 
        fprintf(stderr,"dir failed\n");
        exit(1);
}

/***********************************************************************
test directory operations on a single directory
************************************************************************/
static void *test_dirsingle(int id)
{
        int i;

        for (i=0;i<2000;i++) {
                DIR *d = opendir(".");
                if (!d) goto failed;
                while (readdir(d)) {} ;
                closedir(d);
        }

        return NULL;

failed: 
        fprintf(stderr,"dir failed\n");
        exit(1);
}

/***********************************************************************
test create/unlink operations
************************************************************************/
static void *test_create(int id)
{
	int i;
	char fname[60];
        char dname[30];

        sprintf(dname, "testd_%d", id);
        rmdir(dname);

        if (mkdir(dname, 0777) != 0) goto failed;

	sprintf(fname, "%s/test%d.dat", dname, id);

	for (i=0;i<1000;i++) {
		int fd;
		fd = open(fname, O_CREAT|O_TRUNC|O_RDWR, 0666);
		if (fd == -1) goto failed;
		if (open(fname, O_CREAT|O_TRUNC|O_RDWR|O_EXCL, 0666) != -1) goto failed;
		close(fd);
		if (unlink(fname) != 0) goto failed;
	}
	
        if (rmdir(dname) != 0) goto failed;

	return NULL;

failed:
	fprintf(stderr,"create failed\n");
	exit(1);
}


/***********************************************************************
test fcntl lock operations
************************************************************************/
static void *test_lock(int id)
{
	int i;
	char fname[30];
	int fd;
	sprintf(fname, "test%d.dat", id);

	fd = open(fname, O_CREAT|O_RDWR, 0666);
	if (fd == -1) goto failed;
	unlink(fname);

	for (i=0;i<20000;i++) {
		struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_whence = SEEK_SET;
		lock.l_start = (id*100) + (i%100);
		lock.l_len = 1;
		lock.l_pid = 0;
	
		if (fcntl(fd,F_SETLK,&lock) != 0) goto failed;

		lock.l_type = F_UNLCK;

		if (fcntl(fd,F_SETLK,&lock) != 0) goto failed;
	}

	close(fd);
	
	return NULL;

failed:
	fprintf(stderr,"lock failed\n");
	exit(1);
}


/*
  show the average and range of a set of results
*/
static void show_result(const char *name, double *t, int nrepeats)
{
	double mint, maxt, total;
	int i;
	total = mint = maxt = t[0];
	for (i=1;i<nrepeats;i++) {
		if (t[i] < mint) mint = t[i];
		if (t[i] > maxt) maxt = t[i];
		total += t[i];
	}
	printf("%s  %.2f +/- %.2f seconds\n", name, total/nrepeats, (maxt-mint)/2);
}


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



/* lock a byte range in a open file */
int main(int argc, char *argv[])
{
	int nprocs, i;
	char *tname = "ALL";
#define NREPEATS 10
	struct {
		const char *name;
		void *(*fn)(int );
	} tests[] = {
		{"malloc", test_malloc},
		{"readwrite", test_readwrite},
		{"stat", test_stat},
		{"dir", test_dir},
		{"dirsingle", test_dirsingle},
		{"create", test_create},
		{"lock", test_lock},
		{NULL, NULL}
	};

	if (argc <= 1) {
		printf("thread_perf NPROCS\n");
		exit(1);
	}

	nprocs = atoi(argv[1]);

	if (argc > 2) {
		tname = argv[2];
	}

	for (i=0;tests[i].name;i++) {
		double t_threads[NREPEATS];
		double t_processes[NREPEATS];
		int j;

		if (strcasecmp(tname, "ALL") && strcasecmp(tests[i].name, tname)) {
			continue;
		}

		printf("Running test '%s'\n", tests[i].name);

		for (j=0;j<NREPEATS;j++) {
			start_timer();
			run_threads(nprocs, tests[i].fn);
			t_threads[j] = end_timer();

			start_timer();
			run_processes(nprocs, tests[i].fn);
			t_processes[j] = end_timer();
		}
		show_result("Threads  ", t_threads, NREPEATS);
		show_result("Processes", t_processes, NREPEATS);

		printf("\n");
		fflush(stdout);
	}

	return 0;
}