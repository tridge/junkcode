#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

main()
{
  open("/tmp/cca040331.o", O_WRONLY|O_CREAT|O_TRUNC, 0666);
}

