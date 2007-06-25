#include <stdlib.h>
#include <stdio.h>

#define MAX_ORDER 30
#define FRAME 128

typedef unsigned short uint16;

void lpredict(float *adc, int wsize, float *lpc, int order) 
{
  int   i, j;
  float ci, sum;
  float tmp[MAX_ORDER];
  float acf[MAX_ORDER];

  for(i = 0; i <= order; i++) {
    sum = 0.0;
    for(j = 0; j < wsize - i; j++) sum += adc[j] * adc[j + i];
    acf[i] = sum;
  }
  
  /* find lpc coefficients */
  lpc[0] = 1.0;
  for(i = 1; i <= order; i++) {
    ci = 0.0;
    for(j = 1; j < i; j++) ci += lpc[j] * acf[i-j];
    lpc[i] = ci;
    for(j = 1; j < i; j++) tmp[j] = lpc[j] - ci * lpc[i-j];
    for(j = 1; j < i; j++) lpc[j] = tmp[j];
  }
}

void lpc2signal(float *adc, int wsize, float *lpc, int order) 
{
  int   i, j;
  float ci, sum;

  for(i = 0; i <= order; i++) {
    sum = 0.0;
    for(j = 0; j < wsize - i; j++) sum += adc[j] * adc[j + i];
    acf[i] = sum;
  }
  
  /* find lpc coefficients */
  lpc[0] = 1.0;
  for(i = 1; i <= order; i++) {
    ci = 0.0;
    for(j = 1; j < i; j++) ci += lpc[j] * acf[i-j];
    lpc[i] = ci;
    for(j = 1; j < i; j++) tmp[j] = lpc[j] - ci * lpc[i-j];
    for(j = 1; j < i; j++) lpc[j] = tmp[j];
  }
}


void s_compress(uint16 *udat,float *dat,int framesize)
{
  int i;
  
  /* to floats */
  for (i=0;i<framesize;i++) dat[i] = udat[i];

  lpredict(dat,framesize,lpc,order);

}

main()
{
  float dat[FRAME];
  uint16 udat[FRAME];

  while (!feof(stdin))
    {
      /* read one frame of data */
      if (fread(udat,2,FRAME,stdin) != FRAME) break;

      /* compress it */
      s_compress(udat,dat,FRAME);

      /* uncompress */
      s_uncompress(udat,dat,FRAME);

      /* and write it out */
      fwrite(udat,2,FRAME,stdout);
    }
}
