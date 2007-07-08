/* 
This program provides a set of routines for manipulating waveforms. The
separate routines are accessed by examining argv[0] to see the name of
the routine.

All input and output in these routines are in text format. The 1D format
contains only whitespace separated nimbers. The 2D format is that used
by plot3d (ie. 2 whitespace separated dimensions followed by whitespaced
separated data)

This is designed to be compiles with an ansi compiler such as gcc

Written by :
	Andrew Tridgell
	tridge@aerodec.anu.edu.au

Revision History :
	25-2-91	: started
*/

#include <stdio.h>
#include <malloc.h>
#include <math.h>
#include "speech.h"
#include "manipulate.h"


/* Global Variables */
double FREQUENCY=1000.0; /* Sampling Frequency */
FILE *infile=stdin;
FILE *outfile=stdout;
char PNAME[128];



/* Main Program */
int main(int argc,char *argv[])
{
load_environment();
strcpy(PNAME,basename(argv[0]));
if (strcmp(PNAME,"manipulate")==0)
	return(manipulate_main(argc,argv));
if (strcmp(PNAME,"addabscissa")==0)
	return(addabscissa_main(argc,argv));
if (strcmp(PNAME,"select")==0)
	return(select_main(argc,argv));
if (strcmp(PNAME,"bin2text")==0)
	return(bin2text_main(argc,argv));
if (strcmp(PNAME,"rfft")==0)
	return(rfft_main(argc,argv));
if (strcmp(PNAME,"fftfilter")==0)
	return(fftfilter_main(argc,argv));
if (strcmp(PNAME,"energy")==0)
	return(energy_main(argc,argv));
if (strcmp(PNAME,"zerocross")==0)
	return(zerocross_main(argc,argv));
if (strcmp(PNAME,"prepare")==0)
	return(prepare_main(argc,argv));
if (infile!=stdin) fclose(infile);
if (outfile!=stdout) fclose(outfile);
return(0);
}


/* Functions */


int manipulate_main(int argc,char *argv[])
/* Return error message - should use one of the soft linked files */
{
	fputs("Please call this program via a soft link",stderr);
	return(1);
}


int load_environment(void)
/* Load global variables with environment variables if available */
{
if (getenv("FREQUENCY")) FREQUENCY=atof((char *)getenv("FREQUENCY"));
return(0);
}


int addabscissa_main(int argc,char *argv[])
{
double cur_time=0.0;
double time_step=1.0/FREQUENCY;

/* Look at arguments */
{
int c;
extern char *optarg;
while ((c = getopt(argc, argv, "hf:o:")) != -1)
      switch (c) {
          case 'h':
		/* Help */
	fprintf(stdout,"%s : add an X axis to a sequence of integers\n",PNAME);
	fputs("the environment variable FREQUENCY can be used\n",stdout);
	fputs("to control the X scale.\n",stdout);
	fputs("Both input and output are text\n",stdout);
	file_help();
		return(0);
		break;
	  case 'f':
		if (open_input(optarg)!=0) return(1);
		break;
	  case 'o':
		if (open_output(optarg)!=0) return(1);
		break;
		}
}

while (!feof(infile))
	{
	fprintf(outfile,"%g\t%d\n",cur_time,next_int(infile));
	cur_time+=time_step;
	}
return(0);
}





int select_main(int argc,char *argv[])
/* Used to chop up speech files */
{
double cur_time=0.0;
double time_step=1.0/FREQUENCY;
int multiple=1,i;
int MAXLEN=10000;
long N=1;
long linenum=0;
double Start=0.0;
double End=0.0;
char *s;
/* Look at arguments */
{
int c;
extern char *optarg;
while ((c = getopt(argc, argv, "hf:o:n:s:e:m:")) != -1)
      switch (c) {
          case 'h':
		/* Help */
	fprintf(stdout,"%s : filter to choose sections of a file\n",PNAME);
	fprintf(stdout,"Usage : %s -f INFILE -o OUTFILE -n N -s START -e END\n",PNAME);
	fputs("	-n N		: show every Nth line only\n",stdout);
	fputs("	-mM		: write each line M times\n",stdout);
	fputs("	-s START	: start at START milliseconds\n",stdout);
	fputs("	-s END		: end at END milliseconds\n",stdout);
	file_help();
		return(0);
		break;
	case 'f':
		if (open_input(optarg)!=0) return(1);
		break;
	case 'o':
		if (open_output(optarg)!=0) return(1);
		break;
	case 'n':
		N=atol(optarg);
		if (N==0)
			{
			fprintf(stderr,"Error in N\n");
			return(1);
			}
		break;
	case 's':
		Start=0.001 * atof(optarg);
		break;
	case 'e':
		End=0.001 * atof(optarg);
		break;
	case 'm':
		multiple=atoi(optarg);
		break;
	}
}
s=malloc(MAXLEN);
while (!feof(infile))
	{
	BOOL show_line = (((linenum%N)==0) && (cur_time>=Start) && 
			((cur_time<=End) || (End==0.0)));
	*s=0;
	fgets(s,MAXLEN,infile);
	if (show_line && *s)
		for (i=0;i<multiple;i++) fputs(s,outfile);
	cur_time+=time_step;
	linenum++;
	}
free(s);
return(0);
}


int bin2text_main(int argc,char *argv[])
{
int16 dum1,dum2;
int result;
int byte_skip=512;
BOOL byte_swap=False;
/* A filter which converts binary to text format */
{
int c;
extern char *optarg;
while ((c = getopt(argc, argv, "hf:o:b:s")) != -1)
      switch (c) {
          case 'h':
		/* Help */
	fprintf(stdout,"%s : convert binary file to text\n",PNAME);
	fputs("	-b BYTESKIP	: number of bytes in header\n",stdout);
	fputs("	-s 		: toggle byte swapping (default is off)\n",stdout);
	file_help();
		return(0);
		break;
	case 'f':
		if (open_input(optarg)!=0) return(1);
		break;
	case 'o':
		if (open_output(optarg)!=0) return(1);
		break;
	case 's':
		byte_swap = !byte_swap;
		break;
	case 'b':
		byte_skip=atoi(optarg);
		if (byte_skip<0) 
			{
			fputs("Illegal value\n",stderr);
			return(1);
			}
		break;
		}

}
if (fseek(infile,byte_skip,1)!=0) 
		{
		fputs("Seek error",stderr);
		return(1);
		}	
while (!feof(infile))
	{
	if ((fread(&dum1,sizeof(int16),1,infile))==1)
		{
		if (byte_swap) 
			swab(&dum1,&dum2,sizeof(int16));
		else
			dum2=dum1;
		fprintf(outfile,"%d\n",(int)dum2);
		}
	}
return(0);
}



int rfft_main(int argc,char *argv[])
{
int nn=128;
int skip=128;
int i;
FLOAT *data;
int *idata;
FLOAT pre_emph=0.0;
int dolog=0;
int dohanning=0;
int dohamming=0;
int dobartlett=0;
int average=0;
int dorotate=0;
{
int c;
extern char *optarg;
while ((c = getopt(argc, argv, "hf:o:n:s:w:a:r:p:l")) != -1)
      switch (c) {
          case 'h':
		/* Help */
	fprintf(stdout,"%s : compute a running FFT\n",PNAME);
	fputs("	-nNUM	set the width of each FFT to NUM [64]\n",stdout);
	fputs("	-sSKIP	skip SKIP samples after each FFT [128]\n",stdout);
	fputs("	-aA	moving average over A points on output\n",stdout);
	fputs("	-pA	pre_emphasise with 1-A/z\n",stdout);
	fputs("	-wWIND	apply WIND window.(hamming,hanning,bartlett)\n",stdout);
	fputs("	-rR	rotate window by R before FFT\n",stdout);
	fputs("Note that it takes 2N points to produce a FFT of width N\n",stdout);
	file_help();
		return(0);
		break;
	  case 'f':
		if (open_input(optarg)!=0) return(1);
		break;
	  case 'o':
		if (open_output(optarg)!=0) return(1);
		break;
	  case 'n':
		nn=2*atoi(optarg);
		if (nn<2) 
			{
			fputs("Illegal value for nn\n",stderr);
			return(1);
			}
		break;
	  case 's':
		skip=atoi(optarg);
		if (skip<1) 
			{
			fputs("Illegal value for skip\n",stderr);
			return(1);
			}
		break;
	  case 'p':
		pre_emph=atof(optarg);
		break;
	  case 'l':
		dolog=!dolog;
		break;
	  case 'w':
		if (strncasecmp("HAMMING",optarg,3)==0) dohamming=!dohamming;
		if (strncasecmp("HANNING",optarg,3)==0) dohanning=!dohanning;
		if (strncasecmp("BARTLETT",optarg,1)==0) dobartlett=!dobartlett;
		break;
	  case 'a':
		average=atoi(optarg);
		break;
	  case 'r':
		dorotate=atoi(optarg);
		break;
		}
}

/* Allocate data */
data = malloc(sizeof(FLOAT)*2*nn);
idata = malloc(sizeof(int)*nn);
if (data==NULL || idata==NULL)
	{
	fputs("Error allocating memory\n",stderr);
	return(1);
	}

/* Fill up idata[] */
for (i=0;i<nn;i++)
	idata[i]=next_int(infile);

/* Now loop doing transforms, writing then reading data */
while (!feof(infile))
	{
	int j;

	/* Transfer the data from idata[] to data[] */
	for (j=0;j<nn;j++)
		{
		data[j<<1]=(FLOAT)idata[j];
		data[(j<<1)+1]=0.0;
		}

/* Now apply any windowing functions requested */
if (dohamming) hamming(data,nn,2);
if (dohanning) hanning(data,nn,2);
if (dobartlett) bartlett(data,nn,2);



/* Rotate the data */
if (dorotate) rotate(data,dorotate,nn,2);

/* And pre_emphasise if requested */
if (pre_emph!=0.0) pre_emphasise(data,nn/2,pre_emph,2);

/* Take the fft */	
	fft(data,nn,1);

/* Calculate the modulus */
/* Notice how things move in the array */
	for (j=0;j<nn;j+=2)
		data[j/2]=sqrt(data[j]*data[j]+data[j+1]*data[j+1]);

/* Take a log if requested */
	if (dolog)
	for (j=0;j<nn/2;j++)
		if (data[j]>0.0) data[j]=log(data[j]);

/* Do a moving average if requested */
	if (average>0)
		{
		int k;
		FLOAT sum=0.0;
		int start=0,end=average;

		sum = 0.0;
		for (j=0;j<average;j++) 
			sum += data[j];
		for (j=0;j<average;j++) 
			data[j+(nn/2)] = 0*sum / average;
				

		for (j=average;j<(nn/2 - average);j++)
		{
		sum = 0.0;
		for (k=-average;k<=average;k++)
			sum += data[j+k]/(ABS(k)+1);
		data[j+(nn/2)] = sum / (2*average +1);	
		}

		sum = 0.0;
		for (j=(nn/2 - average);j<(nn/2);j++) 
			sum += data[j];
		for (j=(nn/2 - average);j<(nn/2);j++) 
			data[j+(nn/2)] = 0*sum / average;

		for (j=0;j<(nn/2);j++)
			data[j] = data[j+(nn/2)];

		}
	

	/* write it out */
	for (j=0;j<nn/2;j++)
		fprintf(outfile,"%g ",data[j]);
	fputc('\n',outfile);

	if (skip<=nn)
		{
		for (j=0;j<(nn-skip);j++)
			idata[j]=idata[j+skip];	
		for (j=nn-skip;j<nn;j++)
			idata[j]=next_int(infile);
		}
	else
		{
		for (j=0;j<(skip-nn);j++)
			next_int(infile);
		for (j=0;j<nn;j++)
			idata[j]=next_int(infile);
		}
	}
free(idata);
free(data);
return(0);
}


int fftfilter_main(int argc,char *argv[])
{
int nn=128;
int span=0;
FLOAT blnk_pos=0.0,freq=0.0;
int i;
FLOAT *data,*data1;
int *idata;
int dolog=0;
int dohanning=0;
int dohamming=0;
int dobartlett=0;
int average=0;
int dorotate=0;
int multiples=0;
int firstloop=1;
{
int c;
extern char *optarg;
while ((c = getopt(argc, argv, "hf:o:n:s:w:p:u:m:")) != -1)
      switch (c) {
          case 'h':
		/* Help */
	fprintf(stdout,"%s : remove sections of spectrum\n",PNAME);
	fputs("	-nNUM	set the num of pts in each FFT to NUM [64]\n",stdout);
	fputs("	-pPOS	position in spectrum to blank [0]\n",stdout);
	fputs("	-wWIND	apply WIND window.(hamming,hanning,bartlett)\n",stdout);
	fputs("	-sSPAN	blank all within SPAN of POS [0]\n",stdout);
	fputs("	-mMUL	do MUL multiples of POS also\n",stdout);
	fputs("	-uFREQ	an alternate spec of POS in Hz\n",stdout);
	fputs("		this requires the setting of the FREQUENCY environment variable\n",stdout);

	fputs("	\n",stdout);
	file_help();
		return(0);
		break;
	  case 'f':
		if (open_input(optarg)!=0) return(1);
		break;
	  case 'o':
		if (open_output(optarg)!=0) return(1);
		break;
	  case 'n':
		nn=atoi(optarg);
		if (nn<2) 
			{
			fputs("Illegal value for nn\n",stderr);
			return(1);
			}
		break;
	  case 's':
		span=atoi(optarg);
		break;
	  case 'm':
		multiples=atoi(optarg);
		break;
	  case 'w':
		if (strncasecmp("HAMMING",optarg,3)==0) dohamming=!dohamming;
		if (strncasecmp("HANNING",optarg,3)==0) dohanning=!dohanning;
		if (strncasecmp("BARTLETT",optarg,1)==0) dobartlett=!dobartlett;
		break;
	  case 'p':
		blnk_pos=atof(optarg);
		break;
	  case 'u':
		freq=atof(optarg);
		break;
		}
}


if (freq!=0.0) blnk_pos = (freq/FREQUENCY)*nn;

/* fprintf(stderr,"Filtering : p=%f nn=%d s=%d\n",blnk_pos,nn,span); */

/* Allocate data */
/* fputs("Allocing...\n",stderr);*/
data = malloc(sizeof(FLOAT)*2*nn);
data1=malloc(sizeof(FLOAT)*2*nn);
idata = malloc(sizeof(int)*nn);
if ((data==NULL) || (idata==NULL) || (data1==NULL))
	{
	fputs("Error allocating memory\n",stderr);
	return(1);
	}


	for (i=0;i<nn;i++)
		idata[i]=next_int(infile);
	
	for (i=0;i<nn;i++)
		{
		data[i<<1]=(FLOAT)idata[i];
		data[(i<<1)+1]=0.0;
		}

	for (i=0;i<nn;i++)
		{
		data1[i<<1]=data[i<<1];
		data1[(i<<1)+1]=data[(i<<1)+1];
		}
	
	fft(data1,nn,1);

	for (i=0;i<2*nn;i+=2)
		{
		data1[i] = sqrt(data1[i]*data1[i]+data1[i+1]*data1[i+1]);
		}
	

/* Now loop doing transforms, writing then reading data */
while (!feof(infile))
	{
	int j;


if (!firstloop)
	{
	/* Fill up idata[] */
	for (i=0;i<nn;i++)
		idata[i]=next_int(infile);

	/* Transfer the data from idata[] to data[] */
	for (j=0;j<nn;j++)
		{
		data[j<<1]=(FLOAT)idata[j];
		data[(j<<1)+1]=0.0;
		}
	}
firstloop=0;

/* Now apply any windowing functions requested */
if (dohamming) hamming(data,nn,2);
if (dohanning) hanning(data,nn,2);
if (dobartlett) bartlett(data,nn,2);


	fft(data,nn,1);


/* Now blank the appropriate positions */
/*
	{
	int loops;
	int offset;
	int bp;
	int biggest;
	FLOAT maxmag,mag;
	for (i=1;i<=multiples;i++)
	{
	biggest = 0;
	maxmag = 0.0;
	for (offset=(-span);offset<=span;offset++)
		{
		bp=((int)(i*blnk_pos) + offset)<<1;
		mag = data[bp]*data[bp]+data[bp+1]*data[bp+1];
		if (mag>maxmag)
			{
			maxmag = mag;
			biggest = bp;
			}
		}
		 data[biggest] = 0.0;
		 data[biggest+1] = 0.0;
		 data[2*nn-biggest] = 0.0;
		 data[2*nn-biggest +1] = 0.0;
	}
	}
*/

	{
	FLOAT theta,mmag;
	for(i=0;i<2*nn;i+=2)
		{
		theta = atan2(data[i+1],data[i]);
		mmag = sqrt(data[i]*data[i]+data[i+1]*data[i+1]);
		mmag = mmag - data1[i];
		data[i] = cos(theta)*mmag;
		data[i+1] = sin(theta)*mmag;
		}
	}

/* Now take an inverse fft */
	fft(data,nn,-1);
	
/* Calculate the modulus */
	for (j=0;j<nn;j++)
	idata[j]=(int)(data[j<<1]/nn);


	/* write it out */
	for (j=0;j<nn;j++)
		fprintf(outfile,"%d\n",idata[j]);

	} 
free(idata);
free(data);
free(data1);
return(0);
}


int energy_main(int argc,char *argv[])
{
int dolog=0;
int dohanning=0;
int dohamming=0;
int dobartlett=0;
int width=32;
int skip=16;
int i;
FLOAT *data;
int *idata;
{
int c;
extern char *optarg;
while ((c = getopt(argc, argv, "hf:o:n:s:w:l")) != -1)
      switch (c) {
          case 'h':
		/* Help */
	fprintf(stdout,"%s : compute energy in a signal\n",PNAME);
	fprintf(stdout,
	"	-nNUM	set the width of each sample to NUM [%d]\n",width);
	fprintf(stdout,
	"	-sSKIP	skip SKIP samples after each output [%d]\n",skip);
	fputs("	-l	take log of results\n",stdout);
	fputs("	-wWIND	apply WIND window. (hamming,hanning,bartlett)\n",stdout);
	file_help();
		return(0);
		break;
	  case 'f':
		if (open_input(optarg)!=0) return(1);
		break;
	  case 'o':
		if (open_output(optarg)!=0) return(1);
		break;
	  case 'n':
		width=atoi(optarg);
		break;
	  case 's':
		skip=atoi(optarg);
		if (skip<1) 
			{
			fputs("Illegal value for skip\n",stderr);
			return(1);
			}
		break;
	  case 'l':
		dolog=!dolog;
		break;
	  case 'w':
		if (strncasecmp("HAMMING",optarg,3)==0) dohamming=!dohamming;
		if (strncasecmp("HANNING",optarg,3)==0) dohanning=!dohanning;
		if (strncasecmp("BARTLETT",optarg,1)==0) dobartlett=!dobartlett;
		break;
		}
}

/* Allocate data */
data = malloc(sizeof(FLOAT)*width);
idata = malloc(sizeof(int)*width);
if (data==NULL || idata==NULL)
	{
	fputs("Error allocating memory\n",stderr);
	return(1);
	}

/* Fill up idata[] */
for (i=0;i<width;i++)
	idata[i]=next_int(infile);

/* Now loop doing calculations, writing then reading data */
while (!feof(infile))
	{
	int j;
	FLOAT energy;

/* Transfer the data from idata[] to data[] */
	for (j=0;j<width;j++) data[j]=(FLOAT)idata[j];

/* Now apply any windowing functions requested */
if (dohamming) hamming(data,width,1);
if (dohanning) hanning(data,width,1);
if (dobartlett) bartlett(data,width,1);

/* Now find the energy */

	for (j=0,energy=0.0;j<width;j++) energy+=data[j]*data[j];

/* Do we want log output ? */

if (dolog) energy=log(energy);
	
/* And write it */
	fprintf(outfile,"%g\n",energy);
	
	if (skip<=width)
		{
		for (j=0;j<(width-skip);j++)
			idata[j]=idata[j+skip];	
		for (j=width-skip;j<width;j++)
			idata[j]=next_int(infile);
		}
	else
		{
		for (j=0;j<(skip-width);j++)
			next_int(infile);
		for (j=0;j<width;j++)
			idata[j]=next_int(infile);
		}
	}
free(idata);
free(data);
return(0);
}



int zerocross_main(int argc,char *argv[])
{
int dolog=0;
int width=32;
int skip=16;
int i;
int *idata;
{
int c;
extern char *optarg;
while ((c = getopt(argc, argv, "hf:o:n:s:l")) != -1)
      switch (c) {
          case 'h':
		/* Help */
	fprintf(stdout,"%s : compute average zero crossing rate\n",PNAME);
	fprintf(stdout,
	"	-nNUM	set the width of each sample to NUM [%d]\n",width);
	fprintf(stdout,
	"	-sSKIP	skip SKIP samples after each output [%d]\n",skip);
	fputs("	-l	take log of results\n",stdout);
	file_help();
		return(0);
		break;
	  case 'f':
		if (open_input(optarg)!=0) return(1);
		break;
	  case 'o':
		if (open_output(optarg)!=0) return(1);
		break;
	  case 'n':
		width=atoi(optarg);
		break;
	  case 's':
		skip=atoi(optarg);
		if (skip<1) 
			{
			fputs("Illegal value for skip\n",stderr);
			return(1);
			}
		break;
	  case 'l':
		dolog=!dolog;
		break;
		}
}

/* Allocate data */
idata = malloc(sizeof(int)*width);
if (idata==NULL)
	{
	fputs("Error allocating memory\n",stderr);
	return(1);
	}

/* Fill up idata[] */
for (i=0;i<width;i++)
	idata[i]=next_int(infile);

/* Now loop doing calculations, writing then reading data */
while (!feof(infile))
	{
	int j;
	FLOAT rate;


/* Now find the rate */

	for (j=1,rate=0.0;j<width;j++) rate+=(idata[j-1]*idata[j])<0;
rate = FREQUENCY*rate/width;

/* Do we want log output ? */

if (dolog) rate=log(rate);
	
/* And write it */
	fprintf(outfile,"%g\n",rate);
	
	if (skip<=width)
		{
		for (j=0;j<(width-skip);j++)
			idata[j]=idata[j+skip];	
		for (j=width-skip;j<width;j++)
			idata[j]=next_int(infile);
		}
	else
		{
		for (j=0;j<(skip-width);j++)
			next_int(infile);
		for (j=0;j<width;j++)
			idata[j]=next_int(infile);
		}
	}
free(idata);
return(0);
}



int prepare_main(int argc,char *argv[])
{
int framewidth=400;
int skip=200;
int ncep=0,narc=0,nrc=0;
FLOAT pre_emph=0.0;
FLOAT *cep=NULL,*arc=NULL,*alpha=NULL,*rc=NULL,*cep_saved=NULL,*arc_saved=NULL;

int i;
FLOAT *data;
int *idata;
int dohanning=0;
int dohamming=0;
int docepweighting=0;
int dobartlett=0;
int doinformation=0;
int doerror=0;
int firstpass=1;
{
int c;
extern char *optarg;
while ((c = getopt(argc, argv, "hf:o:n:s:w:p:l:c:r:ieW")) != -1)
      switch (c) {
          case 'h':
		/* Help */
	fprintf(stdout,"%s : prepare a sampled data file\n",PNAME);
	fputs("	-nNUM	set the width of each frame to NUM [400]\n",stdout);
	fputs("	-sSKIP	skip SKIP samples after each frame [200]\n",stdout);
	fputs("	-lNARC	produce NARC auto reg coeffs\n",stdout);
	fputs("	-cNCEP	produce NCEP cepstral coeffs\n",stdout);
	fputs("	-pA	pre_emphasise with 1-A/z\n",stdout);
	fputs("	-i	output information level. Use with -c or -l\n",stdout);
	fputs("	-e	output error level or LPC coeffs. Use with -l\n",stdout);
	fputs("	-rNRC	output reflection coeffs or order NRC\n",stdout);
	fputs("	-wWIND	apply WIND window.(hamming,hanning,bartlett)\n",stdout);
	fputs("	-W	weight cepstrals\n",stdout);
	file_help();
		return(0);
		break;
	  case 'f':
		if (open_input(optarg)!=0) return(1);
		break;
	  case 'o':
		if (open_output(optarg)!=0) return(1);
		break;
	  case 'n':
		framewidth=atoi(optarg);
		break;
	  case 's':
		skip=atoi(optarg);
		if (skip<1) 
			{
			fputs("Illegal value for skip\n",stderr);
			return(1);
			}
		break;
	  case 'l':
		narc=atoi(optarg);
		break;
	  case 'w':
		if (strncasecmp("HAMMING",optarg,3)==0) dohamming=!dohamming;
		if (strncasecmp("HANNING",optarg,3)==0) dohanning=!dohanning;
		if (strncasecmp("BARTLETT",optarg,1)==0) dobartlett=!dobartlett;
		break;
	  case 'p':
		pre_emph=atof(optarg);
		break;
	  case 'c':
		ncep=atoi(optarg)+1;
		break;
	  case 'r':
		nrc=atoi(optarg);
		break;
	  case 'i':
		doinformation=!doinformation;
		break;
	  case 'W':
		docepweighting = !docepweighting;
		break;
	  case 'e':
		doerror=!doerror;
		break;
		}
}

if (nrc>narc) narc=nrc;

/* If doing cepstrals then need auto-reg coeffs */
if ((ncep>0) && (narc==0)) narc=ncep;

if ((ncep!=0) && (ncep<narc))
	{
	fputs("NCEP must be >= (NARC-1) \n",stderr);
	return(1);
	}
if ((ncep==0) && (narc==0) && doinformation)
	{
	fputs("-i can only be used with CEP's or ARC's\n",stderr);
	return(1);
	}
if (doerror && doinformation)
	{
	fputs("Use only one of -e and -i\n",stderr);
	return(1);
	}
if (doerror && (narc==0))
	{
	fputs("You must use -l with -e\n",stderr);
	return(1);
	}


/* Allocate data */
data = malloc(sizeof(FLOAT)*framewidth);
idata = malloc(sizeof(int)*framewidth);
if (data==NULL || idata==NULL)
	{
	fputs("Error allocating memory\n",stderr);
	return(1);
	}

/* Do we need some other space? For arc need arc, alpha and rc space.
   For cep also need cep space.
*/
if (narc>0)
	{
	arc=malloc(sizeof(FLOAT)*(narc+1));
	if (arc==NULL)	{
			fputs("Mem error!\n",stderr);
			return(1);
			}
	alpha=malloc(sizeof(FLOAT)*(narc+1));
	if (alpha==NULL){
			fputs("Mem error!\n",stderr);
			return(1);
			}
	rc=malloc(sizeof(FLOAT)*narc);
	if (rc==NULL){
			fputs("Mem error!\n",stderr);
			return(1);
			}
	if (ncep>0)
	{
	cep = malloc(sizeof(FLOAT)*ncep);
	if (cep==NULL){
			fputs("Mem error!\n",stderr);
			return(1);
			}
	}
 	if (doinformation)
		{
		if (ncep>0)
		cep_saved=malloc(sizeof(FLOAT)*ncep);
		else
		arc_saved=malloc(sizeof(FLOAT)*(narc+1));
		if ((cep_saved==NULL) && (arc_saved==NULL)){
			fputs("Mem error!\n",stderr);
			return(1);
			}
		}
	}

if (doinformation)
	{
	if (ncep>0)
	for(i=0;i<ncep;i++) cep_saved[i]=0.0;
	else
	for(i=0;i<(narc+1);i++) arc_saved[i]=0.0;
	}

/* Fill up idata[] */
for (i=0;i<framewidth;i++)
	idata[i]=next_int(infile);

/* Now loop doing transforms, writing then reading data */
while (!feof(infile))
	{
	int j;

	/* Transfer the data from idata[] to data[] */
	for (j=0;j<framewidth;j++)
		data[j]=(FLOAT)idata[j];

/* And pre_emphasise if requested */
if (pre_emph!=0.0) pre_emphasise(data,framewidth,pre_emph,1);

/* Now apply any windowing functions requested */
if (dohamming) hamming(data,framewidth,1);
if (dohanning) hanning(data,framewidth,1);
if (dobartlett) bartlett(data,framewidth,1);


/* Do we want auto regressive coeffs ? */
if (narc>0)

#ifndef USE_FORTRAN
	ils_auto(framewidth,data,narc,arc,alpha,rc);
#else
	auto_(&framewidth,data,&narc,arc,alpha,rc);  
#endif

/* Do we want cepstrals ? */
if (ncep>0)
#ifndef USE_FORTRAN
	ils_a2cp(arc,cep,narc,ncep);
#else
	a2cp_(arc,cep,&narc,&ncep); 
#endif

if ((ncep>0) && docepweighting)
	for (i=0;i<ncep;i++)
		cep[i] *= 1.0 + 0.5*ncep*sin(Pi*(i+1)/ncep);

/* write it out */

if (narc==0)
	{
	/* we only wanted pre_emph or windowed data */
	for (j=0;j<framewidth;j++)
		fprintf(outfile,"%g ",data[j]);
	fputc('\n',outfile);
	}

if ((narc>0) && (ncep==0) && (nrc==0) && !doinformation && !doerror)
	{
	/* we want the arc's */
	for (j=0;j<narc;j++)
		fprintf(outfile,"%g ",arc[j+1]);
	fputc('\n',outfile);
	}

if ((narc>0) && (ncep==0) && doerror)
	{
	/* we want the error in the ARC predictions */
	fprintf(outfile,"%g\n",arc_error(data,arc,framewidth,narc));
	}

if ((narc>0) && (ncep==0) && (nrc==0) && doinformation && !firstpass)
	fprintf(outfile,"%g\n",sum_sq_diff(arc,arc_saved,narc+1));

if ((narc>0) && (ncep==0) && (nrc>0))
	{
	/* we want the rc's */
	for (j=0;j<nrc;j++)
		fprintf(outfile,"%g ",rc[j]);
	fputc('\n',outfile);
	}

if (ncep>0 && !doinformation)
	{
	/* we want cepstrals */
	for (j=1;j<ncep;j++)
		fprintf(outfile,"%g ",cep[j]);
	fputc('\n',outfile);
	}

if (ncep>0 && doinformation && !firstpass)
	fprintf(outfile,"%g\n",sum_sq_diff(cep,cep_saved,ncep));

if (doinformation)
	{
	if (ncep>0)
	for (i=0;i<ncep;i++) cep_saved[i]=cep[i];
	else
	for (i=0;i<(narc+1);i++) arc_saved[i]=arc[i];
	}	

	if (skip<=framewidth)
		{
		for (j=0;j<(framewidth-skip);j++)
			idata[j]=idata[j+skip];	
		for (j=framewidth-skip;j<framewidth;j++)
			idata[j]=next_int(infile);
		}
	else
		{
		for (j=0;j<(skip-framewidth);j++)
			next_int(infile);
		for (j=0;j<framewidth;j++)
			idata[j]=next_int(infile);
		}
firstpass=False;
	}
free(idata);
free(data);
if (arc!=NULL) free(arc);
if (cep!=NULL) free(cep);
if (alpha!=NULL) free(alpha);
if (rc!=NULL) free(rc);
if (cep_saved!=NULL) free(cep_saved);
if (arc_saved!=NULL) free(arc_saved);
return(0);
}


