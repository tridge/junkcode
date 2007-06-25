#include <stdio.h>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>

int listgroups1(char *user)
{
	struct passwd *pass;
	struct group *grp;
	int ngroups, i;
	gid_t *grps;

	pass = getpwnam(user);

	initgroups(user, pass->pw_gid);

	ngroups = getgroups(0, NULL);

	grps = (gid_t *)malloc(ngroups * sizeof(gid_t));
	
	getgroups(ngroups, grps);

	grp = getgrgid(pass->pw_gid);
	printf("%5d %s\n", pass->pw_gid, grp->gr_name);

	for (i=0;i<ngroups;i++) {
		grp = getgrgid(grps[i]);
		printf("%5d %s\n", grps[i], grp->gr_name);
	}
	return ngroups;
}

int listgroups2(char *user)
{
	struct group *grp;
	char *p;
	int i;

	setgrent();

	while ((grp = getgrent())) {
		if (!grp->gr_mem) continue;
		for (i=0; grp->gr_mem[i]; i++) {
			if (strcmp(grp->gr_mem[i], user) == 0) {
				printf("%s\n", grp->gr_name);
				break;
			}
		}
	}

	endgrent();
}


void listgroups3(char *domain)
{
	struct group *grp;
	int len = strlen(domain);
	
	setgrent();

	while ((grp = getgrent())) {
		if (strncasecmp(grp->gr_name, domain, len) == 0 &&
		    grp->gr_name[len] == '/') {
			printf("%s\n", grp->gr_name);
		}
	}

	endgrent();

}

void listusers(void)
{
	struct passwd *pass;	
	setpwent();
	while ((pass = getpwent())) {
			printf("%s\n", pass->pw_name);
	}
	endpwent();
}


main(int argc, char *argv[])
{
	listusers();
}
