/* quick test of semaphore speed */

/*

Results with the following defines:
  #define NSEMS 20
  #define NPROCS 60
  #define NUMOPS 10000

  OSF1 dominion.aquasoft.com.au V4.0 564 alpha (233 MHz)
  fcntl: 132.814 secs
  ipc: 13.5186 secs

  Linux dominion.aquasoft.com.au 2.0.30 (alpha 233 MHz)
  fcntl: 16.9473 secs
  ipc: 61.9336 secs

  Linux 2.1.57 on a P120
  fcntl: 21.3006 secs
  ipc: 93.9982 secs

  Solaris 2.5 on a Ultra170
  fcntl: 192.805 secs
  ipc: 18.6859 secs

  IRIX 6.4 on a Origin200
  fcntl: 183.488 secs
  ipc: 10.4431 secs

  SunOS 4.1.3 on a 2 CPU sparc 10
  fcntl: 198.239 secs
  ipc: 150.026 secs

  Solaris 2.5.1 on a 4 CPU SPARCsystem-600
  fcntl: 312.025 secs
  ipc: 24.5416 secs

  Linux 2.1.131 on K6/200
  fcntl: 8.27074 secs
  ipc: 49.7771 secs

  Linux 2.0.34 on P200
  fcntl: 7.99596 secs
  ipc: 52.3388 secs

  AIX 4.2 on 2-proc RS6k with 233MHz processors
  fcntl: 12.4272 secs
  ipc: 5.41895 secs

 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/time.h>
#ifdef HAVE_POSIX_SEM
#include <semaphore.h>
#endif

#define NSEMS 20
#define NPROCS 60
#define NUMOPS 10000

#define SEMAPHORE_KEY 0x569890
#define SHM_KEY 0x568698

static int fcntl_fd;
static int sem_id;
static int shm_id;
static int *shm_p;
static int mypid;

#ifdef HAVE_POSIX_SEM
static sem_t posix_sems[NSEMS];
#endif

static int state[NSEMS];          

#define BUFSIZE (1024)
static char buf1[BUFSIZE];
static char buf2[BUFSIZE];

#ifdef NO_SEMUN
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};
#endif


static struct timeval tp1,tp2;

static void start_timer()
{
	gettimeofday(&tp1,NULL);
}

static double end_timer()
{
	gettimeofday(&tp2,NULL);
	return((tp2.tv_sec - tp1.tv_sec) + 
	       (tp2.tv_usec - tp1.tv_usec)*1.0e-6);
}


/************************************************************************
  USING FCNTL LOCKS 
  ***********************************************************************/
void fcntl_setup(void)
{
	fcntl_fd = open("fcntl.locks", O_RDWR | O_CREAT | O_TRUNC, 0600);
	write(fcntl_fd, state, sizeof(state));
}

void fcntl_close(void)
{
	unlink("fcntl.locks");
}

void fcntl_sem_lock(int i)
{
	struct flock lock;
	int ret;

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = i*sizeof(int);
	lock.l_len = sizeof(int);
	lock.l_pid = 0;

	errno = 0;

	if (fcntl(fcntl_fd, F_SETLKW, &lock) != 0) {
		perror("WRLCK");
		exit(1);
	}
}

void fcntl_sem_unlock(int i)
{
	struct flock lock;
	int ret;

	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = i*sizeof(int);
	lock.l_len = sizeof(int);
	lock.l_pid = 0;

	errno = 0;

	if (fcntl(fcntl_fd, F_SETLKW, &lock) != 0) {
		perror("WRLCK");
		exit(1);
	}
}



/************************************************************************
  USING SYSV IPC SEMAPHORES
  ***********************************************************************/
void ipc_setup(void)
{
	int i;
	union semun su;

	su.val = 1;

	sem_id = semget(SEMAPHORE_KEY, NSEMS, 
			IPC_CREAT|IPC_EXCL|0600);
	if (sem_id == -1) {
		sem_id = semget(SEMAPHORE_KEY, 0, 0);
		if (sem_id != -1) {
			semctl(sem_id, 0, IPC_RMID, su);
			sem_id = semget(SEMAPHORE_KEY, NSEMS, 
					IPC_CREAT|IPC_EXCL|0600);
		}
	}
	if (sem_id == -1) {
		perror("semget");
		exit(1);
	}

	for (i=0;i<NSEMS;i++) {
		semctl(sem_id, i, SETVAL, su);
	}

	if (sem_id == -1) {
		perror("semget");
		exit(1);
	}
}

void ipc_close(void)
{
	union semun su;
	su.val = 1;
	semctl(sem_id, 0, IPC_RMID, su);
}


void ipc_change(int i, int op)
{
	struct sembuf sb;
	sb.sem_num = i;
	sb.sem_op = op;
	sb.sem_flg = 0;
	if (semop(sem_id, &sb, 1)) {
		fprintf(stderr,"semop(%d,%d): ", i, op);
		perror("");
		exit(1);
	}
}

void ipc_sem_lock(int i)
{
	ipc_change(i, -1);
}

void ipc_sem_unlock(int i)
{
	ipc_change(i, 1);
}


#ifdef HAVE_POSIX_SEM
/************************************************************************
  USING POSIX semaphores
  ***********************************************************************/
void posix_setup(void)
{
	int i;

	for (i=0;i<NSEMS;i++) {
		if (sem_init(&posix_sems[i], 1, 1) != 0) {
			fprintf(stderr,"failed to setup semaphore %d\n", i);
			break;
		}
	}
}

void posix_close(void)
{
	int i;

	for (i=0;i<NSEMS;i++) {
		if (sem_close(&posix_sems[i]) != 0) {
			fprintf(stderr,"failed to close semaphore %d\n", i);
			break;
		}
	}
}


void posix_lock(int i)
{
	if (sem_wait(&posix_sems[i]) != 0) fprintf(stderr,"failed to get lock %d\n", i);
}

void posix_unlock(int i)
{
	if (sem_post(&posix_sems[i]) != 0) fprintf(stderr,"failed to release lock %d\n", i);
}
#endif


void test_speed(char *s, void (*lock_fn)(int ), void (*unlock_fn)(int ))
{
	int i, j, status;
	pid_t pids[NPROCS];

	for (i=0;i<NSEMS;i++) {
		lock_fn(i);
	}

	for (i=0;i<NPROCS;i++) {
		pids[i] = fork();
		if (pids[i] == 0) {
			mypid = getpid();
			srandom(mypid ^ time(NULL));
			for (j=0;j<NUMOPS;j++) {
				unsigned sem = random() % NSEMS;

				lock_fn(sem);
				memcpy(buf2, buf1, sizeof(buf1));
				unlock_fn(sem);
				memcpy(buf1, buf2, sizeof(buf1));
			}
			exit(0);
		}
	}

	sleep(1);
	
	printf("%s: ", s);
	fflush(stdout);

	start_timer();

	for (i=0;i<NSEMS;i++) {
		unlock_fn(i);
	}

	for (i=0;i<NPROCS;i++)
		waitpid(pids[i], &status, 0);

	printf("%g secs\n", end_timer());
}

int main(int argc, char *argv[])
{
	srandom(getpid() ^ time(NULL));

	printf("NPROCS=%d NSEMS=%d NUMOPS=%d\n", NPROCS, NSEMS, NUMOPS);

	
	fcntl_setup();
	test_speed("fcntl", fcntl_sem_lock, fcntl_sem_unlock);
	fcntl_close();

	ipc_setup();
	test_speed("ipc", ipc_sem_lock, ipc_sem_unlock);
	ipc_close();

#ifdef HAVE_POSIX_SEM
	posix_setup();
	test_speed("posix", posix_lock, posix_unlock);
	posix_close();
#endif

	return 0;
}
