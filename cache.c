#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef SOLARIS2
#include <sys/rusage.h>
#endif

#include <sys/resource.h>
#include <unistd.h>

/*Blocksize is 1M words (on intels, this is 1Mb)
*/

#define MAXBSIZE (1024*1024)
#define LOOPTIME 2.0

typedef unsigned int ui;

double readtime(void);
double read_test(ui);
void build_buffer(int, int);

ui buf[MAXBSIZE * sizeof(ui *)];
FILE *record;

double readtime()
	{
	struct rusage r;
	double rv;

	getrusage(RUSAGE_SELF,&r);

#ifdef SOLARIS2 || _POSIX_C_SOURCE
	rv = (double)r.ru_utime.tv_sec + (double)(r.ru_utime.tv_nsec/1e9);
#else
	rv = (double)r.ru_utime.tv_sec + (double)(r.ru_utime.tv_usec/1e6);
#endif
	return(rv);
	}

main()
	{
	double t,wps;
	int str;
	ui count,buflen;
	int i,pcount=0;
	char strbuf[10];

	/* Open our log file */
	for(i=1; i<32; ++i)
	   {
	   sprintf(strbuf,"cache.%d",i);
	   if (access(strbuf,0)) break;
	   }
	record=fopen(strbuf,"w");
	if (record==NULL) printf("Could not open record file %s. No record kept\n",strbuf);
	else printf("\t\tA copy of this run will be kept in %s\n\n",strbuf);

	printf("Memory tester v1.0\n");
	printf("Author: Anthony.Wesley@anu.edu.au, January 1996\n\n");
	printf("On this machine, one word = %d bytes\n",sizeof(ui *));
	printf("Reported access times are in nS per word.\n\n");
	if (record!=NULL)
	   {
	   fprintf(record,"On this machine, one word = %d bytes\n",sizeof(ui *));
	   fprintf(record,"Reported access times are in nS per word.\n\n");
	   }

	/* Start with a block of 256 words, each word is of type (ui *) */
	build_buffer(256,1);
	count=8;
	do
	   {
	   count *= 2;
	   t = read_test(count);
	   }
	while(t < 2);

	fflush(stdout);

	print_header();
	count = (ui)((double)count * LOOPTIME / t);
	for(buflen=256; buflen<=MAXBSIZE; buflen*=2)
	   {
	   if (buflen<256*1024) sprintf(strbuf,"%uk",buflen/256);
	   else sprintf(strbuf,"%uM",buflen/(256*1024));

	   printf("%-5.5s|",strbuf); fflush(stdout);
	   if (record!=NULL) fprintf(record,"%-5.5s|",strbuf);

	   for(str=1; str<buflen; str*=2)
	   	{
	   	build_buffer(buflen,str);

	   	while(1)
	      	   {
	      	   t = read_test(count);
	      	   if (t<1.0) count*=3; else break;
	      	   }
	
	   	wps = (double)(count) / t;
	  	wps = 1.0e9 / wps;
		if (wps>=1000)
		   {
	    	   printf("1k+ ");
	    	   if (record!=NULL) fprintf(record,"1k+ ");
		   }
		else 
		   {
		   printf("%-3d ", (int)wps);
		   if (record!=NULL) fprintf(record,"%-3d ", (int)wps);
		   }

		fflush(stdout); if (record!=NULL) fflush(record);

	   	count = (ui)((double)count * LOOPTIME / t);
	   	}
	   putchar('\n');
	   if (record!=NULL) putc('\n',record);
	   }

	if (record!=NULL) fclose(record);
	}

double read_test(ui count)
	{
	double t1;
	register ui *o,i=0;

	o=(ui *)buf[0];
	t1=readtime(); while(i<count)
	   	{
		o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o);
		o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o);
		o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o);
		o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o);
		o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o);
		o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o);
		o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o);
		o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o);
		o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o);
		o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o);
		o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o);
		o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o);
		o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o);
		o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o);
		o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o);
		o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o); o=(ui *)(*o);
 		i+=64;
	   	}
	return(readtime()-t1);
	}

void
build_buffer(int size, int stridelen)
	{
	ui i;

	for(i=0; i<(size-stridelen); i+=stridelen)
		buf[i]=(ui)&buf[i+stridelen];
	buf[i] = (ui)&buf[0];
	}

print_header()
	{
	char strbuf[32];
	unsigned v,val,i;
	char c=0;

	printf(
"Block|--------------------------Memory Stride (words)------------------------\n");
printf("size |");

	if (record!=NULL)
	   {
	   fprintf(record,
	   "Block|--------------------------Memory Stride (words)------------------------\n");
	   fprintf(record,"size |");
	   }

	for(val=1; val<=(1024*128); val*=2)
	   {
	   v=val;
	   if (val>=(1024*1024)) { c='M'; v = val / (1024*1024); }
	   else if (val>=1024) { c='k'; v = val / 1024; }

	   sprintf(strbuf,"%d%c",v,c);
	   if (v>64 && c) {strbuf[2]=c; strbuf[3]=0;}
	   printf("%-3s ",strbuf);
	   if (record!=NULL) fprintf(record,"%-3s ",strbuf);
	   }
	putchar('\n'); if (record!=NULL) putc('\n',record);
	printf("     |\n");
	if (record!=NULL) fprintf(record,"     |\n");
	}
