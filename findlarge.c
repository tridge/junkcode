#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>


void findit(int size, char *dir)
{
  DIR *d;
  struct dirent *de;

  d = opendir(dir);
  if (!d) return;


  while ((de = readdir(d))) {
    char *fname;
    struct stat st;

    if (strcmp(de->d_name,".")==0) continue;
    if (strcmp(de->d_name,"..")==0) continue;

    fname = (char *)malloc(strlen(dir) + strlen(de->d_name) + 2);
    if (!fname) {
      fprintf(stderr,"out of memory\n");
      exit(1);
    }
    sprintf(fname,"%s/%s", dir, de->d_name);

    if (lstat(fname, &st)) {
      perror(fname);
      continue;
    }

    if (st.st_size >= size) {
      printf("%s %dk\n", fname, (int)(st.st_size/1024));
    }

    if (S_ISDIR(st.st_mode)) {
      findit(size, fname);
    }

    free(fname);
  }

  closedir(d);
  
}


int main(int argc, char *argv[])
{
  int size;

  if (argc < 3) {
    fprintf(stderr,"%s: <minsize> <dir>\n", argv[0]);
    exit(1);
  }

  size = atoi(argv[1]);

  findit(size, argv[2]);
  return 0;
}
