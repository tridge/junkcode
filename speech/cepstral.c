
#include <stdio.h>
#include <malloc.h>

#include "speech.h"

int cepstral(int numpts,FLOAT *data,int ncep,int narc,FLOAT *cep,FLOAT *arc);
void ils_auto(int n,FLOAT *x,int m,FLOAT *a,FLOAT *alpha,FLOAT *rc);
void ils_a2cp(FLOAT *a,FLOAT *c,int m,int nc);


/* 
	This routine takes sampled data in *data and 
	produces the cepstral coefficients and autoregressive 
	coefficients. The dimensions of the arrays are
	data[numpts], cep[ncep] and arc[narc+1].
	Note that ncep>=narc ?
*/

int cepstral(numpts,data,ncep,narc,cep,arc)
int numpts,ncep,narc;
FLOAT *data,*cep,*arc;
{
FLOAT *alpha,*rc;

rc = (FLOAT *)malloc(sizeof(FLOAT)*narc);
if (rc==NULL) return(-1);
alpha = malloc(sizeof(FLOAT)*(narc+1));
if (alpha==NULL) {free(rc);return(-1);}

ils_auto(numpts,data,narc,arc,alpha,rc);
ils_a2cp(arc,cep,narc,ncep);

free(rc);
free(alpha);
return(0);
}



void ils_auto(n,x,m,a,alpha,rc)
int n;
FLOAT *x;
int m;
FLOAT *a,*alpha,*rc;
{
/*
C...
C...  CALCULATE INVERSE FILTER COEFFICIENTS USING LEVINSON'S METHOD.
C...  SEE MARKEL AND GRAY [1976, PG. 219] FOR DESCRIPTION.
C...  MODIFIED TO RETAIN ALPHA TERMS FOR EACH ITERATION.
C...
C...  DESCRIPTION OF ARGUMENTS
C...
C...  N       NUMBER OF POINTS
C...  X       VECTOR OF SAMPLED DATA POINTS
C...  M       ORDER OF INVERSE FILTER
C...  A       VECTOR OF INVERSE FILTER COEFFICIENTS
C...  ALPHA   VECTOR OF ENERGY TERMS FOR EACH ITERATION
C...  RC      VECTOR OF REFLECTION COEFFICIENTS
C...
*/

/*
      dimension x(*),a(*),rc(*),alpha(*)
      dimension r(30)
*/
FLOAT *r,sum,alpmin,s,rcminc,aip,aib;
int mp,k,km1,npend,np,idx,minc,minc1,minc2,ip,mh,ib;

/*
C...
C...  DO AUTOCORRELATION
C...
*/
      mp=m+1;
	r = (FLOAT *)malloc(sizeof(FLOAT)*(mp+5)); /* Don't really need
							+5,+2 is OK */

/*
      do 120 k=1,mp
        km1=k-1
        npend=n-km1
        sum=0.
        do 110 np=1,npend
          idx=np+km1
          sum=sum+x(np)*x(idx)
110     continue
        r(k)=sum
120   continue
*/
for(k=1;k<=mp;k++)
	{
	km1=k-1;
	npend=n-km1;
	sum=0.0;
	for (np=1;np<=npend;np++)
		{
		idx=np+km1;
		sum+=x[np-1]*x[idx-1];
		}
	r[k-1]=sum;
	}

/*
C...
C...  DO SOLVE ROUTINE
C...
*/
/*
      a[1]=1.
      alpha[1]=r[1]
      if (m.eq.0) go to 170
      if (r[1].le.0.0) go to 170
      rc[1]=-r[2]/r[1]
      a[2]=rc[1]
      alpha[2]=r[1]+r[2]*rc[1]
      if (m.eq.1) go to 170
*/
a[0]=1.0;
alpha[0]=r[0];
if ((m==0) || (r[0]<=0.0)) {free(r);return;}
rc[0] = -r[1]/r[0];
a[1]=rc[0];
alpha[1]=r[0]+r[1]*rc[0];
if (m==1) {free(r);return;}

/*
      do 150 minc=2,m
        alpmin=alpha[minc]
        minc1=minc+1
        minc2=minc+2
        s=0.
*/
for (minc=2;minc<=m;minc++)
	{
	alpmin=alpha[minc-1];
	minc1=minc+1;
	minc2=minc+2;
	s=0.0;

/*
        do 130 ip=1,minc
          idx=minc2-ip
          s=s+r[idx]*a[ip]
130     continue
*/
	for (ip=1;ip<=minc;ip++)
		{
		idx=minc2-ip;
		s+=r[idx-1]*a[ip-1];
		}
/*
        rcminc=-s/alpmin
        rc[minc]=rcminc
        mh=minc/2+1
*/
	rcminc = -s/alpmin;
	rc[minc-1]=rcminc;
	mh=minc/2+1;

/*
        do 140 ip=2,mh
          ib=minc2-ip
          aip=a[ip]
          aib=a[ib]
          a[ip]=aip+rcminc*aib
          a[ib]=aib+rcminc*aip
140     continue
*/
	for(ip=2;ip<=mh;ip++)
		{
		ib=minc2-ip;
		aip=a[ip-1];
		aib=a[ib-1];
		a[ip-1]=aip+rcminc*aib;
		a[ib-1]=aib+rcminc*aip;
		}

/*
        a[minc1]=rcminc
        alpha[minc1]=alpmin-alpmin*(rcminc*rcminc)
        if (alpha[minc1].le.0.0) go to 160
*/
	a[minc1-1]=rcminc;
	alpha[minc1-1]=alpmin-alpmin*(rcminc*rcminc);
	if (alpha[minc1-1]<=0.0) {free(r);return;}

	}
/*
150   continue
C...
160   continue
C...
170   return
      end
*/
{free(r);return;}
}



/*      subroutine a2cp(a,c,m,nc)
C...
C...  SUBROUTINE FOR EFFICIENT COMPUTATION
C...  OF CEPSTRAL COEFFS
C...
C...
C...  A       AUTOREGRESSIVE COEFFICIENTS (INPUT)
C...  C       CEPSTRAL COEFFICIENTS (OUTPUT)
C...  M       ORDER OF AUTOREGRESSIVE POLYNOMIAL (INPUT)
C...  NC      ORDER OF CEPSTRAL POLYNOMIAL (MAY BE.GT.M)(INPUT)
C...
*/
void ils_a2cp(a,c,m,nc)
FLOAT *a,*c;
int m,nc;
{
int i,ip,j,jj,mp1,np; 
FLOAT ap,suma;
/*
      dimension a(*),c(*)
*/

/*
C...
C...  INITIAL CONDITIONS
C...
      c(1)=0.
      c(2)=-a(2)
*/
	c[0]=0.0;
	c[1]= -a[1];
/*
C...
C...  COMPUTE FIRST M TERMS
C...
      do 120 i=2,m
      ip=i+1
      ap=float(i)
      suma=ap*a(ip)
      do 110 j=2,i
      jj=(i-j)+2
110   suma=suma+a(j)*c(jj)
120   c(ip)=-suma
*/
	for(i=2;i<=m;i++)
		{
		ip=i+1;
		ap=(FLOAT)i;
		suma=ap*a[ip-1];
		for(j=2;j<=i;j++)
			{
			jj=(i-j)+2;
			suma+=a[j-1]*c[jj-1];
			}
		c[ip-1]= -suma;
		}

/*
C...
C...  COMPUTE TERMS FROM M+1 OUT TO NC
C...
      mp1=m+1
      np=nc+1
      if (nc.le.m) go to 150
      do 140 i=mp1,nc
      ip=i+1
      suma=0.
*/
	mp1=m+1;
	np=nc+1;
	if(nc>m) 
{
	for(i=mp1;i<=nc;i++)
		{
		ip=i+1;
		suma=0.0;
		for(j=2;j<=mp1;j++)
			{
			jj=(i-j)+2;
			suma+=a[j-1]*c[jj-1];
			}
		c[ip-1]= -suma;
		}
/*
      do 130 j=2,mp1
      jj=(i-j)+2
130   suma=suma+a(j)*c(jj)
140   c(ip)=-suma
*/
}

/*
150   do 160 j=3,np
160   c(j)=c(j)/float(j-1)
*/
	for(j=3;j<=np;j++)
		c[j-1] /= (FLOAT)(j-1);
/*
      return
      end
*/
}


