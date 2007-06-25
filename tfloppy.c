#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

main(int argc,char *argv[])
{
  int count = 1024*1024;
  char *fname = argv[1];
  int fd1,fd2;

  fd1 = open("/dev/fd0",O_WRONLY,0);
  fd2 = open(fname,O_WRONLY|O_CREAT|O_TRUNC,0666);

  while (count--)
    {
      char val = random();
      int pos = (unsigned)random() % (1440*1024);
      int doit = (random() % 1000 == 0);
      lseek(fd1,pos,SEEK_SET);
      lseek(fd2,pos,SEEK_SET);
      write(fd1,&val,sizeof(val));
      write(fd2,&val,sizeof(val));
      if (doit)
	sync();
    }
}
  
