#include <stdio.h>
#include <math.h>
#include <sys/time.h>


#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

/* for solaris */
#ifdef SOLARIS
#include <limits.h>
#include <sys/times.h>
struct tms tp1,tp2;

static void start_timer()
{
  times(&tp1);
}

static double end_timer()
{
  times(&tp2);
  return((tp2.tms_utime - tp1.tms_utime)/(1.0*CLK_TCK));
}

#elif (defined(LINUX))

#include <sys/resource.h>
struct rusage tp1,tp2;

static void start_timer()
{
  getrusage(RUSAGE_SELF,&tp1);
}


static double end_timer()
{
  getrusage(RUSAGE_SELF,&tp2);
  return((tp2.ru_utime.tv_sec - tp1.ru_utime.tv_sec) + 
	 (tp2.ru_utime.tv_usec - tp1.ru_utime.tv_usec)*1.0e-6);
}

#else

#include <sys/time.h>

struct timeval tp1,tp2;

static void start_timer()
{
  gettimeofday(&tp1,NULL);
}

static double end_timer()
{
  gettimeofday(&tp2,NULL);
  return((tp2.tv_sec - tp1.tv_sec) + 
	 (tp2.tv_usec - tp1.tv_usec)*1.0e-6);
}

#endif

static FILE *f;

static void dumpval(double val)
{
  fwrite((void *)&val,sizeof(val),1,f);
}



static void memcpy_test(int size)
{
  int i;
  int loops = 5.0e8 / size;
  char *p1 = (char *)malloc(size);
  char *p2 = (char *)malloc(size);
  double t;

  memset(p2,42,size);
  start_timer();

  for (i=0;i<loops;i++)
    {
      memcpy(p1,p2,size);
      memcpy(p2,p1,size);
    }

  t = end_timer();
  dumpval(*(double *)p1);
  dumpval(*(double *)p2);
  free(p1); free(p2);

  printf("%g Mb/S\n",i*2.0*size/(1.0e6*t));
}


main()
{
  int loops = 100000;
  int l;
  double d=0;
  double t;
  int i;

#define ADD_SIZE 1*1024
  
  f = fopen("/dev/null","w");
  
  printf("Floating point - sin() - ");
  l = 1024;
  start_timer();
  for (i=0;i<loops;i++)
    d += sin((double)i);
  t = end_timer();
  printf("%g MOPS\n",loops/(1.0e6*t));
  dumpval(d);

  printf("Floating point - log() - ");
  l = 1024;
  start_timer();
  for (i=0;i<loops;i++)
    d += log((double)(i+1));
  t = end_timer();
  printf("%g MOPS\n",loops/(1.0e6*t));
  dumpval(d);

  printf("Memcpy - 1kB - ");
  memcpy_test(1024);

  printf("Memcpy - 100kB - ");
  memcpy_test(1024*100);

  printf("Memcpy - 1MB - ");
  memcpy_test(1024*1024);

  printf("Memcpy - 10MB - ");
  memcpy_test(1024*1024*10);

  loops *= 10;
  printf("Adding integers - ");
  l = ADD_SIZE;
  {
    int *p1 = (int *)malloc(l*sizeof(int));
    int sum;
    for (i=0;i<l;i++)
      p1[i] = i;
    start_timer();
    for (i=0;i<loops/100;i++)
      {
	int j;
	sum = i;
	for (j=0;j<l;j++)
	  sum += p1[j];
      }
    t = end_timer();
    dumpval(sum);
    free(p1);
  }
  printf("%g MOPS\n",0.01*(loops*l)/(1.0e6*t));

  printf("Adding floats (size %d) - ",sizeof(float));
  l = ADD_SIZE;
  {
    float *p1 = (float *)malloc(l*sizeof(float));
    float sum;
    for (i=0;i<l;i++)
      p1[i] = i;
    start_timer();
    for (i=0;i<loops/100;i++)
      {
	int j;
	sum = i;
	for (j=0;j<l;j++)
	  sum += p1[j];
      }
    t = end_timer();
    dumpval(sum);
    free(p1);
  }
  printf("%g MOPS\n",0.01*(loops*l)/(1.0e6*t));

  printf("Adding doubles (size %d) - ",sizeof(double));
  l = ADD_SIZE;
  {
    double *p1 = (double *)malloc(l*sizeof(double));
    double sum;
    for (i=0;i<l;i++)
      p1[i] = i;
    start_timer();
    for (i=0;i<loops/100;i++)
      {
	int j;
	sum = i;
	for (j=0;j<l;j++)
	  sum += p1[j];
      }
    t = end_timer();
    dumpval(sum);
    free(p1);
  }
  printf("%g MOPS\n",0.01*(loops*l)/(1.0e6*t));

}
