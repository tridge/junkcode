#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>


void main(int argc,char *argv[])
{
char *devname0 = argv[1];
char *devname1 = argv[2];
fd_set fds;
int fd0,fd1;
int select_return;
int read_return;
char buf[10];

fd0 = open(devname0,O_RDWR | O_NONBLOCK);
fd1 = open(devname1,O_RDWR | O_NONBLOCK);

FD_ZERO(&fds);
FD_SET(fd0,&fds);
FD_SET(fd1,&fds);

select_return = select(255,&fds,NULL,NULL,NULL);

printf("select() gave %d\n",select_return);

if (FD_ISSET(fd0,&fds)) 
  printf("read() of fd0 gave %d\n",read(fd0,buf,10));

if (FD_ISSET(fd1,&fds)) 
  printf("read() of fd1 gave %d\n",read(fd1,buf,10));

close(fd0);
close(fd1);
}
  
    
