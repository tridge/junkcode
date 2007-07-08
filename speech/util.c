/* Utilities needed in speech processing programs */

#include <stdio.h>
#include <math.h>
#include "speech.h"

extern FILE *infile;
extern FILE *outfile;

int next_int(FILE *file)
/* Read an integer string from file and return value */
{
int dum;
fscanf(file,"%d",&dum);
return(dum);
}

int open_input(char *optarg)
{
if ((infile=fopen(optarg,"r"))==NULL)
	{
	fprintf(stderr,"Error opening %s for input\n",optarg);
	return(1);
	}
return(0);
}

int open_output(char *optarg)
{
if ((outfile=fopen(optarg,"w"))==NULL)
	{
	fprintf(stderr,"Error opening %s for output\n",optarg);
	return(1);
	}
return(0);
}

int file_help(void)
{
fputs("	-f INFILE	: name of input file (stdin default)\n",stdout);
fputs("	-o OUTFILE	: name of output file (stdout default)\n",stdout);
fputs("	-h		: help\n",stdout);
}


char *basename(char *s)
/* Same as basename[1] - strips path */
{
int len=strlen(s);
while (--len)
	if (s[len-1]=='/') break;
return((char *)(s+len));
}

/* 
Window functions -
Multiples data[] by a window. n specifies the number of data points.
Skip is included to allow for manipulating only the real parts of a 
complex array. In this case skip would be 2. Normally skip should be 1.
*/

void hamming(FLOAT *data,int n,int skip)
{
int j;
for (j=0;j<n;j++)
	data[j*skip]*=0.54+0.46*cos(Pi*(2.0*j/(n-1)-1.0));
}

void hanning(FLOAT *data,int n,int skip)
{
int j;
for (j=0;j<n;j++)
	data[j*skip]*=0.5+0.5*cos(Pi*(2.0*j/(n-1)-1.0));
}

void bartlett(FLOAT *data,int n,int skip)
{
int j;
for (j=0;j<n;j++)
	data[j*skip]*=1.0-fabs((2.0*j)/(n-1)-1);
}

void rotate(FLOAT *data,int num,int n,int skip)
/* rotate array by num steps */
{
int j,k;
FLOAT tmp1,tmp2;
tmp1=data[0];
for (j=0;j<n;j++)
	{
	k = ((j*num)%n)*skip;
	tmp2=data[k];
	data[k]=tmp1;
	tmp1=tmp2;
	}
}

void pre_emphasise(FLOAT *data,int n,FLOAT pre_emph,int skip)
{
int j;
FLOAT temp1,temp2;
temp1=data[0];
for (j=1;j<n;j++)
	{
	temp2=data[j*skip];
	data[j*skip] -= pre_emph*temp1;
	temp1=temp2;
	}
}


FLOAT sum_sq(FLOAT *data,int n)
{
int i;
FLOAT sum=0.0;
for (i=0;i<n;i++) sum += data[i]*data[i];
return(sum);
}

FLOAT sum_sq_diff(FLOAT *data1,FLOAT *data2,int n)
{
int i;
FLOAT sum=0.0;
FLOAT diff;
for (i=0;i<n;i++) 
	{
	diff = data2[i]-data1[i];
	sum += diff*diff;
	}
return(sum);
}

FLOAT arc_error(FLOAT *data,FLOAT *arc,int numpts,int narc)
/* Find the error in the predicted data from the given arc's */
/* NOTE : arc is arc[narc+1] */
{
FLOAT err=0.0,val,mag=0.0;
int i,j;
if (narc>=numpts) return(0.0);
for (i=narc;i<numpts;i++)
	{
	for (val=0.0,j=0;j<=narc;j++)
		val += data[i-j]*arc[j];
	mag += fabs(data[i]);
	err += fabs(val);
	}
return(err*(numpts-narc)/mag);
}


