#include <stdio.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>

int main(void)
{
	struct group *grp;
	int ngroups, i;
	gid_t *grps;

	ngroups = getgroups(0, NULL);

	grps = (gid_t *)malloc(ngroups * sizeof(gid_t));
	
	getgroups(ngroups, grps);

	grp = getgrgid(getegid());
	printf("%5d %s (primary)\n", getegid(), grp->gr_name);

	for (i=0;i<ngroups;i++) {
		grp = getgrgid(grps[i]);
		printf("%5d %s\n", grps[i], grp?grp->gr_name:"<NULL>");
	}
	return 0;
}
