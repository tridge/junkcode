#include <stdio.h>
#include <string.h>

int main(void)
{
  char *test = "1234567890";
  int i;
  for (i=0;i<9;i++) {
    printf("%d\n", strnlen(test,i));
  }
  return 0;
}

