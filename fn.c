#include <stdlib.h>
#include <fnmatch.h>

int main()
{
  char *pattern = "a/*/e";
  char *path = "a/b/c/d/e";
  int i;

  i = fnmatch(pattern,path,0);
  printf("fnmatch w/out FNM_PATHNAME: %d\n",i);
  i = fnmatch(pattern,path,FNM_PATHNAME);
  printf("fnmatch w/ FNM_PATHNAME: %d\n",i);
  return 0;
}
