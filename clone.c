#include <sched.h>
#include <unistd.h>

static void fn1(void)
{
	sleep(1);
	system("/bin/pwd");
}

main()
{
	char stack[81920];
	clone(fn1, stack+80000 , CLONE_FS, NULL);
	chdir("/tmp");
	sleep(2);
	system("/bin/pwd");
}
