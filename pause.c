#include <stdio.h>

main(int argc,char *argv[])
{
  char *s = "Press any key to continue....";
  if (argc > 1)
    s = argv[1];
  puts(s);
  getc(stdin);
}
