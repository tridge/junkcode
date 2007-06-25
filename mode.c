#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

struct stat buf;

int mode(file)
char *file;
{
buf.st_mode = 0;
stat(file,&buf);
return(buf.st_mode);
}

main(argc,argv)
int argc;
char *argv[];
{
if (argc>1) printf("%o\n",mode(argv[1]));
return(0);
}

