#include <stdio.h>
#include <math.h>
#include "speech.h"


/* 	FFT routine from Numerical Recipes.
	Both input and output data is passed through data[], the 
	transform is done in situ.
	nn is the number of complex numbers in data[]. Thus there are
	2*nn FLOATs in data[]
	isign is 1 for a forward transform, -1 for reverse

	Call fft(,,) for C arrays, four1(,,) for [1..N] arrays
*/



#define SWAP(a,b) tempr=(a);(a)=(b);(b)=tempr


void four1(FLOAT *data,int nn,int isign);

void fft(data,nn,isign)
/* This allows calling with C arrays */
FLOAT data[];
int nn,isign;
{
four1(&(data[-1]),nn,isign);
}



void four1(data,nn,isign)
FLOAT data[];
int nn,isign;
{
	int n,mmax,m,j,istep,i;
	double wtemp,wr,wpr,wpi,wi,theta;
	FLOAT tempr,tempi;

	n=nn << 1;
	j=1;
	for (i=1;i<n;i+=2) {
		if (j > i) {
			SWAP(data[j],data[i]);
			SWAP(data[j+1],data[i+1]);
		}
		m=n >> 1;
		while (m >= 2 && j > m) {
			j -= m;
			m >>= 1;
		}
		j += m;
	}
	mmax=2;
	while (n > mmax) {
		istep=2*mmax;
		theta=6.28318530717959/(isign*mmax);
		wtemp=sin(0.5*theta);
		wpr = -2.0*wtemp*wtemp;
		wpi=sin(theta);
		wr=1.0;
		wi=0.0;
		for (m=1;m<mmax;m+=2) {
			for (i=m;i<=n;i+=istep) {
				j=i+mmax;
				tempr=wr*data[j]-wi*data[j+1];
				tempi=wr*data[j+1]+wi*data[j];
				data[j]=data[i]-tempr;
				data[j+1]=data[i+1]-tempi;
				data[i] += tempr;
				data[i+1] += tempi;
			}
			wr=(wtemp=wr)*wpr-wi*wpi+wr;
			wi=wi*wpr+wtemp*wpi+wi;
		}
		mmax=istep;
	}
}

#undef SWAP



