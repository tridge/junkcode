#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>



int aoct_int(char *s)
{
int l = strlen(s);
int i;
int res=0;
for (i=0;i<l;i++)
	{
	int v;
	char c = s[l-i-1];
	if (c=='0') continue;
	v = c-'1' + 1;
	res |= (v<<(i*3));
	}
return(res);
}


main(argc,argv)
int argc;
char *argv[];
{
if (argc>2) chmod(argv[2],aoct_int(argv[1]));
return(0);
}

