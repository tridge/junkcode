main()
{
int s=1024*1024;
char *p;
while ((p=(char *)malloc(s))) bzero(p,s);
}
