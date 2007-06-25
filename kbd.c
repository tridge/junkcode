#include <stdio.h>
#include <fcntl.h>
#include <sys/kd.h>

main()
{
int kbd = open("/dev/console", O_RDWR | O_NDELAY);
char buf[60];
int n,i;

ioctl(kbd, KDSKBMODE, K_RAW);

while (1)
  {
    n = read(kbd,buf,60);
    if (n==0) continue;
    for (i=0;i<n;i++)
      printf("%x\n",buf[i]);
  }

}
