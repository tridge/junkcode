#include <stdio.h>
#include <signal.h>
#include <unistd.h>

static void CatchSignal(int signum,void (*handler)(int ));

void BlockSignals(int block,int signum)
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set,signum);
	sigprocmask(block?SIG_BLOCK:SIG_UNBLOCK,&set,NULL);
}


static void handler(int sig)
{
	BlockSignals(1, sig);
	sigblock(sigmask(sig));
	printf("got signal\n");
}

static void CatchSignal(int signum,void (*handler)(int ))
{
	struct sigaction act;

	act.sa_handler = handler;
	act.sa_flags = SA_RESTART|SA_NODEFER;

	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask,signum);
	sigaction(signum,&act,NULL);
}


int main(void)
{
	CatchSignal(SIGUSR1, handler);

	while (1) {
		sleep(100);
	}
	return 0;
}
