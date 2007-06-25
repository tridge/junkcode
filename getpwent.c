#include <stdio.h>
#include <pwd.h>
#include <sys/types.h>

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
