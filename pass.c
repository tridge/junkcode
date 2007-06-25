#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <pwd.h>
#include <unistd.h>
#include <pwd.h>

#ifdef SHADOW_PWD
#include <shadow.h>
#endif


void main()
{
  char *password;
#ifdef SHADOW_PWD
  struct spwd *spass = NULL;
#endif
  struct passwd *pass = NULL;
  char salt[100];
  char user[100];

  printf("Username: ");
  if (scanf("%s",user) != 1)
    exit(0);
      
  password = getpass("Password: ");
  pass = getpwnam(user);
  if (pass == NULL)
    printf("couldn't find account %s\n",user); 
  else
    {
      int pwok = 0;
#ifdef PWDAUTH
      pwok = (pwdauth(user,password) == 0);
#else
#ifdef SHADOW_PWD
      spass = getspnam(user);
      if (spass && spass->sp_pwdp)
	pass->pw_passwd = spass->sp_pwdp;
#endif
      strncpy(salt,pass->pw_passwd,2);
      salt[2] = 0;
      pwok = (strcmp(crypt(password,salt),pass->pw_passwd) == 0);
#endif
      if (!pwok)
	printf("invalid password\n");
      else
	printf("password OK\n");
    }
}
