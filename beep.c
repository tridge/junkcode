#include <stdio.h>

main(int argc,char *argv[])
{
  int count=1;
  if (argc > 1)
    count = atoi(argv[1]);
  while (count--)
    {
      putc(7,stdout);
      fflush(stdout);
      usleep(200000);
    }
}
