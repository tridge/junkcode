#include <stdio.h>
#include <sys/fcntl.h>

main()
{
  char buf[100];
  int fd;
  
  mkfifo("ttyc",0777);
  fd = open("ttyc",O_RDWR);

  read(fd,buf,8);
  printf("got [%s]\n",buf);

  write(fd,"world!\n",7);
  printf("done\n");
}

