#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

struct stat buf;

int group(file)
char *file;
{
buf.st_mode = 0;
stat(file,&buf);
return(buf.st_gid);
}

main(argc,argv)
int argc;
char *argv[];
{
if (argc>1) printf("%d\n",group(argv[1]));
return(0);
}

