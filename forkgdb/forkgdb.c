/*
  compile like this:

  gcc -c -fPIC forkgdb.c
  ld -shared -o forkgdb.so forkgdb.o -ldl
  
  LD_PRELOAD=/usr/local/lib/forkgdb.so myapp ARGS

*/

#define _GNU_SOURCE

#include <sys/types.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

pid_t fork(void)
{
	static pid_t (*real_fork)(void);
	pid_t ret;
	char *cmd;
	char *env;

	if (!real_fork) {
		real_fork = dlsym((void *)-1, "fork");
	}

	ret = real_fork();

	if (ret != 0) {
		return ret;
	}

	asprintf(&cmd, "xterm -e 'gdb /proc/%d/exe %d' &", getpid(), getpid());

	env = getenv("LD_PRELOAD");

	unsetenv("LD_PRELOAD");

	signal(SIGCHLD, SIG_IGN);

	system(cmd);

	setenv("LD_PRELOAD", env, 1);

	return ret;
}
