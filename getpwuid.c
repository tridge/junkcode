#include <stdio.h>
#include <pwd.h>
#include <sys/types.h>

static void print_passwd(struct passwd *pwd)
{
	printf("%s:%s:%d:%d:%s:%s:%s\n", 
	       pwd->pw_name,
	       pwd->pw_passwd,
	       pwd->pw_uid,
	       pwd->pw_gid,
	       pwd->pw_gecos,
	       pwd->pw_dir,
	       pwd->pw_shell);
}

int main(int argc, char *argv[])
{
	struct passwd *pwd;

	pwd = getpwuid(atoi(argv[1]));

	if (!pwd) {
		printf("Failed to fetch pwd\n");
	} else {
		print_passwd(pwd);
	}
	return 0;
}
