#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

/* return a pointer to a anonymous shared memory segment of size "size"
   which will persist across fork() but will disappear when all processes
   exit 
   The memory is not zeroed 
   */
void *shm_setup(int size)
{
	int shmid;
	void *ret;

	shmid = shmget(IPC_PRIVATE, size, SHM_R | SHM_W);
	if (shmid == -1) {
		printf("can't get shared memory\n");
		exit(1);
	}
	ret = (void *)shmat(shmid, 0, 0);
	if (!ret || ret == (void *)-1) {
		printf("can't attach to shared memory\n");
		return NULL;
	}

	/* the following releases the ipc, but note that this process
	   and all its children will still have access to the memory, its
	   just that the shmid is no longer valid for other shm calls. This
	   means we don't leave behind lots of shm segments after we exit */
	shmctl(shmid, IPC_RMID, 0);
	
	return ret;
}



int main(int argc, char *argv[])
{
	char *buf;
	int size, child;

	if (argc < 2) {
		printf("shm_sample <size>\n");
		exit(1);
	}

	size = atoi(argv[1]);

	buf = shm_setup(size);

	if (!buf) {
		printf("shm_setup(%d) failed\n", size);
		exit(1);
	}
	memset(buf, size, 0);

	child = fork();

	/* now just to show it worked we will put the pid of the child
           at the start of the shared memory segment and read it from
           the child */
	if (child) {
		*(int *)buf = child;
	} else {
		while (*(int *)buf == 0) ;
		printf("pid via shm is %d real pid is %d\n",
		       *(int *)buf, getpid());
	}

	return 0;
}

