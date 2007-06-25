#include <stdio.h>
#include <math.h>


main()
{
  int i;
  double d;
  for (i=0;i<10000;i++)
    d += sin(1.0*i);
}
