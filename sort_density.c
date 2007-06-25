#include <stdio.h>
#include <stdlib.h>

#define N_PER_BIN 20000
#define BINS 10
#define HBINS 20

float bins[BINS][N_PER_BIN];

float frandom(float low,float high)
{
  unsigned long r = random();
  r = r % (2<<20);
  return low + (high - low)*((double)r)/(2<<20);
}

void create_bin(float *bin)
{
  int i;
  for (i=0;i<N_PER_BIN;i++) bin[i] = frandom(0.0,1.0);
}

void create_bins(void)
{
  int i;
  for (i=0;i<BINS;i++)
    create_bin(bins[i]);
}


int cmp(float *f1,float *f2)
{
  if (*f1 > *f2) return 1;
  if (*f1 < *f2) return -1;
  return 0;
}

void merge(float *bin1,float *bin2)
{
  float combined[N_PER_BIN*2];

  memcpy(combined,bin1,sizeof(bin1[0])*N_PER_BIN);
  memcpy(combined+N_PER_BIN,bin2,sizeof(bin1[0])*N_PER_BIN);
  qsort(combined,2*N_PER_BIN,sizeof(combined[0]),cmp);

  memcpy(bin1,combined,sizeof(bin1[0])*N_PER_BIN);
  memcpy(bin2,combined+N_PER_BIN,sizeof(bin1[0])*N_PER_BIN);
  
}


show_density(float *bin,char *name)
{
  float hist[HBINS];
  int i;
  FILE *f;

  for (i=0;i<HBINS;i++) hist[i] = 0;

  for (i=0;i<N_PER_BIN;i++) {
    int j = (int)((bin[i]-0.0001) / 0.05);
    hist[j]++;
  }
  for (i=0;i<HBINS;i++) {
    hist[i] = (hist[i]*HBINS)/(float)N_PER_BIN;
  }

  f = fopen("hist.dat","w");
  for (i=0;i<HBINS;i++) 
    fprintf(f,"%f\n",hist[i]);
  fclose(f);

  f = popen("gnuplot","w");
  if (!f) {
    printf("failed to launch gnuplot\n"); 
    return;
  }
  fprintf(f,"
set yrange [0:2]
set data style linespoints
plot \"hist.dat\"
");

  fflush(f);
  getchar();
  fclose(f);
}


main(void)
{
  create_bins();

  merge(bins[0],bins[1]);

  show_density(bins[0],"none");
  show_density(bins[1],"none");
}
