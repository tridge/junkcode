#include <unistd.h>
#include <stdio.h>


int main(int argc,char *argv[])
{
int delay = 30;
if (argc>1)
  delay = atoi(argv[1]);

while (1)
  {
    sleep(delay);
    sync();
  }
return(0);
}

