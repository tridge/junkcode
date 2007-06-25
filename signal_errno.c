#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

static volatile int foo;

static void sigalrm(int num)
{
	errno = 123;
	foo=1;
}

int main(int argc, char *argv[])
{
	errno = 0;
	write(23, NULL, 0);
	alarm(1);
	signal(SIGALRM, sigalrm);
	write(23, NULL, 0);
	while (!foo) ;
	printf("errno=%d\n", errno);
	return 0;
}
