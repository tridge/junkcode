static void hyper_sort(int base,int number,void (*makesmaller)())
{
  int i;
  if (number==1) return;
  for (i=0;i<(number/2);i++)
    makesmaller(base+i,base+i+((number+1)/2));

  hyper_sort(base+number/2,(number+1)/2,makesmaller);
  hyper_sort(base,number - (number+1)/2,makesmaller);
}


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

#define MAX_STEPS 10000
struct pair {
  int n1,n2,step;
};

struct pair steps[MAX_STEPS];

static int N=0;

void makesmaller(int n1,int n2)
{
  steps[N].n1 = n1;
  steps[N].n2 = n2;
  steps[N].step = N;
  N++;
}


int conflict(struct pair *p,int step)
{
  int i;
  for (i=0;i<N;i++)
    if (step == steps[i].step && 
	(steps[i].n1 == p->n1 ||
	 steps[i].n1 == p->n2 ||
	 steps[i].n2 == p->n1 ||
	 steps[i].n2 == p->n2))
      return(1);
  return(0);
}

void reduce()
{
  int changed;

  do {
    int i;
    changed = 0;

    for (i=0;i<N;i++) {
      if (steps[i].step>0 && !conflict(&steps[i],steps[i].step-1)) {
	steps[i].step--;
	changed=1;
      }
    }

  } while (changed);
}


int comparison(struct pair *p1,struct pair *p2)
{
  if (p1->step != p2->step) return(p1->step - p2->step);
  if (p1->n1 != p2->n1) return(p1->n1 - p2->n1);
  if (p1->n2 != p2->n2) return(p1->n2 - p2->n2);
  return(0);
}

void printout()
{
  int i;
  int step=0;
  for (i=0;i<N;i++) {
    if (steps[i].step != step) {
      printf("\n");
      step++;
    }
    printf("%d:  %d %d\n",steps[i].step,steps[i].n1,steps[i].n2);
  }
  printf("\n%d steps\n",step+1);
}


main(int argc,char *argv[])
{
  batchers_sort(0,atoi(argv[1]),makesmaller);

  reduce();

  qsort(&steps[0],N,sizeof(struct pair),comparison);

  printout();
}
