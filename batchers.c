static void batchers_sort(int baseP,int N,void (*makesmaller)())
{
  int	p, initq, q, r, d, x;

  for (p=1; (1<<(p+1))<=N+1; p++);
  p = 1<<p;
  
  for (initq=p; p>0; p/=2) 
    {
      q = initq;
      r = 0;
      d = p;
      do 
	{
	  for (x=0; x<N-d; x++)
	    if ( (x & p) == r) 
	      makesmaller(baseP+x,baseP+x+d);
	  d = q - p;
	  q /= 2;
	  r = p;
	} while (q != p/2);
    }
}

void makesmaller(int n1,int n2)
{
  printf("%d %d\n",n1,n2);
}

main(int argc,char *argv[])
{
  batchers_sort(0,atoi(argv[1]),makesmaller);
}
