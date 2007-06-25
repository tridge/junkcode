#include <stdio.h>
#include <dirent.h>


main()
{
  DIR *d = opendir(".");
  readdir(d);
}
