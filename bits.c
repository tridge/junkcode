#include <stdio.h>


bits(unsigned int x)
{
  int i;
  for (i=0;i<8;i++)
    if (x & (1<<i)) 
      putchar('1');
    else
      putchar('0');
}

main()
{
  unsigned char x;
  for (x='a';x<='z';x++)
    {
      printf("%c    ",x);
      bits(x);
      printf("    ");
      bits(toupper(x));
      printf("\n");
    }

}
