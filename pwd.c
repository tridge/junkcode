#include <stdio.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>


int pwdauth(char *user,char *password)
{
  struct passwd *pass = getpwnam(user);
  if (!pass) {
    errno = EINVAL; /* Note: SunOS returns EACCES */
    return(-1);
  }
  if (strcmp(crypt(password,pass->pw_passwd),pass->pw_passwd)) {
    errno = EACCES;
    return(-1);
  }
  return(0);
}

int grpauth(char *group,char *password)
{
  struct group *grp = getgrnam(user);
  if (!grp) {
    errno = EINVAL;
    return(-1);
  }
  if (grp->gr_passwd &&
      strcmp(crypt(password,grp->gr_passwd),grp->gr_passwd)) {
    errno = EACCES;
    return(-1);
  }
  return(0);
}


main(int argc,char *argv[])
{
  char *pass = getpass("Password: ");


  errno=0;
  printf("res=%d (%s,%s)\n",pwdauth(argv[1],pass),argv[1],pass);
  printf("errno=%d\n",errno);
}
