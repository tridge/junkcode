#include <stdlib.h>
#include <stdio.h>


void report()
{
printf("uid=%d euid=%d\n",getuid(),geteuid());
}

main()
{
report();
seteuid(65534);
report();
seteuid(-2);
report();
}
