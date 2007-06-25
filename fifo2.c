#include <stdio.h>
#include <sys/fcntl.h>

main()
{
  char buf[100];
  int fd;

  sleep(5);
  fd = open("ttyc",O_RDWR);

  write(fd,"helloxxY",8);

  read(fd,buf,7);
  printf("got [%s]\n",buf);
}

