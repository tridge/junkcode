/*
  a replacement for getgrent() on AIX that enumerates 
  all user databases, not just /etc/group

  Andrew Tridgell tridge@au.ibm.com
  February 2004
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grp.h>
#include <usersec.h>
#include <sys/types.h>


static struct {
	char *all_groups;
	char *next;
} getgrent_state;


/*
  free up any resources associated with getgrent() 
*/
void endgrent(void)
{
	if (getgrent_state.all_groups) {
		free(getgrent_state.all_groups);
	}
	getgrent_state.all_groups = NULL;
	getgrent_state.next = NULL;
}


/*
  reset the state to the start of the group database
*/
void setgrent(void)
{
	char *s;
	endgrent();
	/* getgroupattr returns a null separated list of groups,
	   with each group being of the form REGISTRY:GROUPNAME */
	if (getgroupattr("ALL", "groups", &s, SEC_CHAR) == 0) {
		getgrent_state.all_groups = s;
		getgrent_state.next = getgrent_state.all_groups;
	}
	
}

/*
  get the next group
*/
struct group *getgrent(void)
{
	char *group;
	struct group *grp;
	if (!getgrent_state.all_groups) {
		setgrent();
		if (!getgrent_state.all_groups) {
			return NULL;
		}
	}
	do {
		group = getgrent_state.next;
		if (!*group) {
			endgrent();
			return NULL;
		}
		group = strchr(group, ':');
		if (!group) {
			endgrent();
			return NULL;
		}
		grp = getgrnam(group+1);
		getgrent_state.next += strlen(getgrent_state.next)+1;
	} while (!grp);

	return grp;
}


#if TEST_PROGRAM
int main(void)
{
	struct group *grp;

	setgrent();

	while ((grp = getgrent())) {
		printf("%s\n", grp->gr_name);
	}
	
	endgrent();

	return 0;
}
#endif
