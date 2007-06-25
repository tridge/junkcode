#include <stdio.h>


main()
{
  if (getuid() != 0) {
    printf("this test program must be run as root\n");
    exit(1);
  }

  printf("uid=%d euid=%d\n",getuid(),geteuid());

  setreuid (148,-1) || setreuid (-1,148);


  printf("uid=%d euid=%d\n",getuid(),geteuid());

  sleep(100);

  setreuid (-1,0) || setreuid (0,-1);

  printf("uid=%d euid=%d\n",getuid(),geteuid());

}
