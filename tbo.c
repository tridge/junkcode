#include <stdio.h>

main()
{
short d = 255;
unsigned char *p = &d;
printf("%x %x\n",(unsigned int)p[0],(unsigned int)p[1]);
}
