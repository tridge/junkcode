#define NGROUPS_MAX 1000

#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <string.h>
#include <grp.h>
#include <pwd.h>

#ifndef HAVE_GETGROUPLIST
/*
  This is a *much* faster way of getting the list of groups for a user
  without changing the current supplemenrary group list. The old
  method user getgrent() which could take 20 minutes on a really big
  network with hundeds of thousands of groups and users. The new method
  takes a couple of seconds. (tridge@samba.org)

  unfortunately it only works if we are root!
  */
int
getgrouplist(uname, agroup, groups, grpcnt)
	const char *uname;
	gid_t agroup;
	register gid_t *groups;
	int *grpcnt;
{
	gid_t gids_saved[NGROUPS_MAX + 1];
	int ret, ngrp_saved;
	
	ngrp_saved = getgroups(NGROUPS_MAX, gids_saved);
	if (ngrp_saved == -1) {
		return -1;
	}

	if (initgroups(uname, agroup) != 0) {
		return -1;
	}

	ret = getgroups(*grpcnt, groups);
	if (ret >= 0) {
		*grpcnt = ret;
	}

	if (setgroups(ngrp_saved, gids_saved) != 0) {
		/* yikes! */
		fprintf(stderr,"getgrouplist: failed to reset group list!\n");
		exit(1);
	}

	return ret;
}
#endif

int main(int argc, char *argv[])
{
	char *user = argv[1];
	struct passwd *pwd;
	gid_t gids[NGROUPS_MAX + 1];
	int count, ret, i;

	pwd = getpwnam(user);

	if (!pwd) {
		printf("Unknown user '%s'\n", user);
		exit(1);
	}

	count = 11;

	ret = getgrouplist(user, pwd->pw_gid, gids, &count);

	printf("ret=%d\n", ret);

	if (ret == -1) {
		printf("getgrouplist failed\n");
		return -1;
	}

	printf("Got %d groups\n", ret);
	if (ret != -1) {
		for (i=0;i<count;i++) {
			printf("%u ", (unsigned)gids[i]);
		}
		printf("\n");
	}
	
	return 0;
}
