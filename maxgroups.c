#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>


static int trygroups(int n)
{
	gid_t *gids;
	int i;

	gids = malloc(sizeof(gid_t) * n);
	for (i=0;i<n;i++) {
		gids[i] = i;
	}
	if (setgroups(n, gids) != 0) {
		free(gids);
		return -1;
	}

	memset(gids, 0, sizeof(gid_t) * n);

	if (getgroups(n, gids) != n) {
		free(gids);
		return -1;
	}

	free(gids);
	return 0;
}


int main(int argc, char *argv[])
{
	int i;
	
	if (geteuid() != 0) {
		printf("You must run this as root\n");
		exit(1);
	}


	for (i=0;i<1000000;i++) {
		if (trygroups(i) != 0) {
			printf("\nmax of %d supplementary groups\n", i-1);
			exit(0);
		}
		printf("%d\r", i);
		fflush(stdout);
	}
	printf("\nno limit on supplementary groups!?\n");
	return 0;
}
