#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>


int main(void)
{
	gid_t gid = (gid_t)-2;

	if (setegid(gid) != 0) {
		perror("setegid");
		exit(1);
	}
	
	errno = 0;

	gid = getegid();
	
	if (gid == (gid_t)-1 && errno == 2) {
		printf("oh oh - returned a gid as an errno?!\n");
		exit(1);
	}

	if (gid != (gid_t)-2) {
		printf("Failed to set egid!?\n");
		exit(1);
	}

	if (errno != 0) {
		printf("getegid overwrote errno - bad stuff\n");
		exit(1);
	}


	return 0;
}
