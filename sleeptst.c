#include <stdio.h>
#include <sys/time.h>


main()
{
struct timeval tp1;
struct timeval tp2;
int i;

for (i=0;i<10;i++)
  {

    gettimeofday(&tp1,NULL);
    usleep(1*i);
    gettimeofday(&tp2,NULL);

    printf("diff=%d\n",
	   (tp2.tv_sec-tp1.tv_sec)*1000000 + (tp2.tv_usec-tp1.tv_usec));
  }
}
