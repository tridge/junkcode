#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>

int listgroups(char *user)
{
	struct passwd *pass;
	struct group *grp;
	int ngroups, i;
	gid_t *grps;

	pass = getpwnam(user);

	if (!pass) {
		printf("Unknown user '%s'\n", user);
		exit(1);
	}

	if (initgroups(user, pass->pw_gid) != 0) {
		perror("initgroups");
		exit(1);
	}

	ngroups = getgroups(0, NULL);

	if (ngroups == 0) {
		printf("Using is in no groups!\n");
		exit(1);
	}

	grps = (gid_t *)malloc(ngroups * sizeof(gid_t));
	
	if (getgroups(ngroups, grps) != ngroups) {
		printf("Failed to get group list!\n");
		exit(1);
	}

	grp = getgrgid(pass->pw_gid);

	if (pass->pw_gid != grps[0]) {
		printf("%5d %s\n", pass->pw_gid, grp?grp->gr_name:"UNKNOWN");
	}

	for (i=0;i<ngroups;i++) {
		grp = getgrgid(grps[i]);
		printf("%5d %s\n", grps[i], grp?grp->gr_name:"UNKNOWN");
	}
	return ngroups;
}



int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage: initgroups USERNAME\n");
		exit(1);
	}

	if (geteuid() != 0) {
		printf("This program must be run as root\n");
		exit(1);
	}


	listgroups(argv[1]);
	return 0;
}
