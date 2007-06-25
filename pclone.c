/* test the clone syscall with new flags */
#include <stdio.h>
#include <signal.h>
#include <linux/sched.h>
#include <linux/unistd.h>

_syscall2(int, clone, int, flags, int, sp);

main()
{
  printf("pid=%d\n",getpid());

  /* clone(SIGCLD|CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND,0);  */
  clone(SIGCLD|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_PID|CLONE_VM,0);
  printf("pid=%d\n",getpid());
}
