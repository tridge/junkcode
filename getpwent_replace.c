/*
  a replacement for getpwent() on AIX that enumerates 
  all user databases, not just /etc/passwd

  Andrew Tridgell tridge@au.ibm.com
  February 2004
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <usersec.h>
#include <sys/types.h>


static struct {
	char *all_users;
	char *next;
} getpwent_state;


/*
  free up any resources associated with getpwent() 
*/
void endpwent(void)
{
	if (getpwent_state.all_users) {
		free(getpwent_state.all_users);
	}
	getpwent_state.all_users = NULL;
	getpwent_state.next = NULL;
}


/*
  reset the state to the start of the user database
*/
void setpwent(void)
{
	char *s;
	endpwent();
	/* getuserattr returns a null separated list of users,
	   with each user being of the form REGISTRY:USERNAME */
	if (getuserattr("ALL", "users", &s, SEC_CHAR) == 0) {
		getpwent_state.all_users = s;
		getpwent_state.next = getpwent_state.all_users;
	}
	
}

/*
  get the next user
*/
struct passwd *getpwent(void)
{
	char *user;
	struct passwd *pwd;
	if (!getpwent_state.all_users) {
		setpwent();
		if (!getpwent_state.all_users) {
			return NULL;
		}
	}
	do {
		user = getpwent_state.next;
		if (!*user) {
			endpwent();
			return NULL;
		}
		user = strchr(user, ':');
		if (!user) {
			endpwent();
			return NULL;
		}
		pwd = getpwnam(user+1);
		getpwent_state.next += strlen(getpwent_state.next)+1;
	} while (!pwd);

	return pwd;
}


#if TEST_PROGRAM
int main(void)
{
	struct passwd *pwd;

	setpwent();

	while ((pwd = getpwent())) {
		printf("%s\n", pwd->pw_name);
	}
	
	endpwent();

	return 0;
}
#endif
