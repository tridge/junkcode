#include <stdlib.h>
#include <signal.h>
#include <sys/resource.h>


void sig_segv()
{
  signal(SIGSEGV,SIG_DFL);
  printf("got segv!\n");
  return;
}

main()
{
  {
    struct rlimit rlp;
    getrlimit(RLIMIT_CORE, &rlp);
    rlp.rlim_cur = 4*1024*1024;
    setrlimit(RLIMIT_CORE, &rlp);
    getrlimit(RLIMIT_CORE, &rlp);
    printf("Core limits now %d %d\n",rlp.rlim_cur,rlp.rlim_max);
  }


  signal(SIGSEGV,sig_segv);
  *(char *)0 = 1;
}
