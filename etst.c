#include <errno.h>


main()
{
  int x=0;
  errno = 0;
  write(7,&x,sizeof(x));
  printf("errno=%d error=%s errno=%d\n",errno,strerror(errno),errno);
  write(1,&x,0);
  printf("errno=%d error=%s errno=%d\n",errno,strerror(errno),errno);
  printf("hello\n");
  printf("errno=%d error=%s errno=%d\n",errno,strerror(errno),errno);
  printf("errno=%d error=%s errno=%d\n",errno,strerror(errno),errno);
}
