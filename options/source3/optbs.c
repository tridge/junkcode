/* module BS - black-scholes calculations */


#include <math.h>
#include "optdef.h"
#include "options.h"


/*
void ndf(double x,double *n,double *h)
{
static double c[5] = {1.33027,-1.82126,1.78148,-0.35656,0.31938};
static double a = 0.39894;
static double b = 0.23164;

double  u,y;
int     i;
     *h = 0.0;
     if (x > 13.0)
	 *n = 1.0;
     else
         if (x <-13.0)
	 *n = 0.0;
     else
       {
          u = 1.0/(1.0 + b * (double)fabs(x));
          *h = a*exp(-0.5*x*x);
          y = 0.0;
          for (i = 0;i<5;i++)
              y = u * (y + c[i]);
          if (x > 0)  *n = 1.0 - *h*y;
                   else  *n = *h*y;
     }
}
*/


double ndf(double x)
{
static double a = 0.39894;
static double b = 0.33267;
double  u,y;
          u = 1.0/(1.0 + b * (double)fabs(x));
          y = ((0.937299*u - 0.1201676)*u + 0.4361836)*u;
          if (x > 0)     return(1.0 - a*exp(-0.5*x*x)*y);
                   else  return(a*exp(-0.5*x*x)*y);
}


void initdivmtx(int future)
{
int count;
int r;
int currentdays;
int y;
int m;
int d;

count = 0;
r=0;
divmatrix[0].s = 0.0;
convertdate(sys.date, &y, &m, &d);
currentdays = serialdate(y,m,d) + future;

while (serialdate(status.sizepay[8+r].dyear,
       status.sizepay[8+r].dmonth, status.sizepay[8+r].dday) - currentdays <= 0
       && r < 12)
 r++;

while (r < 12)
    {
    if (status.sizepay[8+ r].payout > 0 && status.sizepay[8+r].dday != 0)
     {
       divmatrix[count].days= serialdate(status.sizepay[8+r].dyear,
       status.sizepay[8+r].dmonth, status.sizepay[8+r].dday) - currentdays;
       divmatrix[count+1].s = divmatrix[count].s -
			status.sizepay[8+r].payout* 0.01 * exp( 0.00274 *
			status.interest * -0.01 * divmatrix[count].days) ;
       count++;
     }
     r++;
   }
  divmatrix[count].days = 380;
  divmatrix[count+ 1].days = 0;
  divmatrix[count+ 1].s = 0.0 ;
  /* terminator */
}


/*
void BSdiv(volpct,strike,stock,rpct,days,valuec,valuep,deltac,deltap)
double volpct,strike,stock,rpct,*valuec,*valuep,*deltac,*deltap;
int days;
{
double callmax;
double putmax;
double deltapmax;
double deltacmax;
int r;
r = 0;
callmax = 0;
putmax = 0;
   while (days >= divmatrix[r].days)
     {
        BS(volpct,strike,stock + divmatrix[r].s,rpct,divmatrix[r].days,
           &(*valuec),&(*valuep),&(*deltac),&(*deltap));
        if ( *valuec >= callmax)
	{
	  callmax = *valuec;
	  deltacmax = *deltac;

	}
	BS(volpct,strike,stock + divmatrix[r+1].s,rpct,divmatrix[r].days,
           &(*valuec),&(*valuep),&(*deltac),&(*deltap));
        if ( *valuep >= putmax)
	{
	  putmax = *valuep;
	  deltapmax = *deltap;

	}

	r++;
      }
    BS(volpct,strike,stock + divmatrix[r].s,rpct,days,&(*valuec),&(*valuep),&(*deltac)
	  ,&(*deltap));
	if ( *valuec >= callmax)
	{
	  callmax = *valuec;
	  deltacmax = *deltac;
	}
	else
	{
	*valuec = callmax;
	*deltac = deltacmax;
	}
	if ( *valuep >= putmax)
	{
	  putmax = *valuep;
	  deltapmax = *deltap;
	}
	else
	{
	*valuep = putmax;
	*deltap = deltapmax;
	}
}
*/


void PBSdiv(volpct,strike,stock,rpct,days,valuep,deltap)
double volpct,strike,stock,rpct,far *valuep,far *deltap;
int days;
{
double deltapmax;
double putmax;
int r;
r = 0;
putmax = 0;
   while (days >= divmatrix[r].days)
     {
        PBS(volpct,strike,stock + divmatrix[r].s,rpct,divmatrix[r].days,&(*valuep),&(*deltap));
        if ( *valuep >= putmax)
	{
	  putmax = *valuep;
	  deltapmax = *deltap;

	}

	r++;
      }
    PBS(volpct,strike,stock + divmatrix[r].s,rpct,days,&(*valuep),&(*deltap));
	if ( *valuep >= putmax)
	{
	  putmax = *valuep;
	  deltapmax = *deltap;
	}
	else
	{
	*valuep = putmax;
	*deltap = deltapmax;
	}
}


void CBSdiv(volpct,strike,stock,rpct,days,valuec,deltac)
double volpct,strike,stock,rpct,far *valuec,far *deltac;
int days;
{
double callmax;
double deltacmax;
int r;
r = 0;
callmax = 0;
   while (days >= divmatrix[r].days)
     {
        CBS(volpct,strike,stock + divmatrix[r].s,rpct,divmatrix[r].days,
           &(*valuec),&(*deltac));
        if ( *valuec >= callmax)
	{
	  callmax = *valuec;
	  deltacmax = *deltac;

	}
	r++;
      }
   CBS(volpct,strike,stock + divmatrix[r].s,rpct,days,&(*valuec),&(*deltac));
	if ( *valuec >= callmax)
	{
	  callmax = *valuec;
	  deltacmax = *deltac;
	}
	else
	{
	*valuec = callmax;
	*deltac = deltacmax;
	}
}






void PBS(volpct,strike,stock,rpct,days,valuep,deltap)
double volpct,strike,stock,rpct,far *valuep,far *deltap;
int days;
{
double intrinsicval;
double tjul,vt,vol;
double   z1,n1,n2,e1,e2,q,r;
volpct *= 1.0;
rpct *= 0.41;
intrinsicval= strike - stock;
if (strike <= 0.0 || stock <= 0)
{*valuep = 0.0;*deltap = 0;}
else
{
if (days < 0) tjul = 0.0;
else
tjul = 0.00274*days;

r = rpct*0.01;
q = strike*exp(-r*tjul);
vol = volpct*0.01;
vt = vol*sqrt(tjul);


  if (tjul > 0.0025 && vt > 0.0 && q > 0)
  {
     z1 = log(stock/q)/vt + vt*0.5;
     n1 = ndf(z1);
     n2 = ndf(z1-vt);
     *valuep = stock*(n1-1) + (1-n2)*q;
     *deltap = n1-1;
  }

  else

  {
     if (stock > strike)
     {
	  *deltap = 0.0;
	  *valuep = 0.0;
     }
     else
     {
          *valuep = strike - stock;
	  *deltap = -1.0;
     }
  }

}
if (intrinsicval > *valuep)
{
*valuep = intrinsicval;
*deltap = -1.0;
}
}

void CBS(volpct,strike,stock,rpct,days,valuec,deltac)
double volpct,strike,stock,rpct,far *valuec,far *deltac;
int days;
{
double tjul,q,vt,r,vol;
double   z1,n1,n2,e1,e2;
if (strike <= 0.0 || stock <= 0)
{*valuec = 0.0; *deltac = 0.0;}
else
{
if (days < 0) tjul = 0.0;
else
tjul = 0.00274*days;
r = rpct*0.01;
vol = volpct*0.01;
q = strike*exp(-r*tjul);
vt = vol*sqrt(tjul);


  if (tjul > 0.0025 && vt > 0.0 && q > 0.0)
  {
     z1 = log(stock/q)/vt + vt*0.5;
     n1 = ndf(z1);
     n2 = ndf(z1-vt);
     *valuec = stock * n1 - q * n2;
     *deltac = n1;
  }

  else

  {
     if (stock > strike)
     {
          *deltac = 1.0;
          *valuec = stock-strike;
     }
     else
     {
          *deltac = 0.0;
          *valuec = 0.0;
     }
  }

}
}


void calcrow(int row)
{
int days;
days = dtx(status.data[row-8].month);
CBSdiv(status.volatility,
   status.data[row-8].strike,
   status.stockprice,
   status.interest,
   days,
   &(status.data[row-8].valuec),
   &(status.data[row-8].deltac));
PBSdiv(status.volatility,
   status.data[row-8].strike,
   status.stockprice,
   status.interest,
   days,
   &(status.data[row-8].valuep),
   &(status.data[row-8].deltap));
}


void calcall()
{
int i;
for (i=8;i<37;i++)
calcrow(i);
}



double invertvolc(int row)
{
int days,count;
double ivalue,diff,error,newcall,
	dum,call1,slope,cvalue,vol,delvol;

     days = dtx(status.data[row-8].month);
     cvalue = status.data[row-8].marketc;
     diff = status.stockprice-status.data[row-8].strike;
     if (diff > 0) ivalue = diff; else ivalue = 0;
     if ((cvalue >= status.stockprice) || (cvalue < 0.008) || (cvalue < ivalue))
     return(-1);
     vol = 99.5;
     count = 12;
          do
	  {
CBSdiv(vol-0.5,
   status.data[row-8].strike,
   status.stockprice,
   status.interest,
   days,
   &call1,
   &dum);
CBSdiv(vol+0.5,
   status.data[row-8].strike,
   status.stockprice,
   status.interest,
   days,
   &newcall,
   &dum);
   slope = newcall-call1;
   newcall = (newcall + call1)*0.5;
		error = fabs(newcall-cvalue)/cvalue;
                if (slope < 1e-10)
                  {
                  error = 0;
                  delvol = 1;
                  }
                 else
                  delvol = (cvalue - newcall)/slope;
                vol += delvol;
		count--;
          } while ((error > 0.0001) && (count !=0));
          if (count == 0) return(-1);
	  if (fabs(delvol) > 0.1) return(-2);
            return(vol);
}


double invertvolp(int row)
{
int days,count;
double strike,error,newput,
	dum,put1,slope,pvalue,vol,delvol;

     days = dtx(status.data[row-8].month);
     pvalue = status.data[row-8].marketp;
     if ((pvalue >= status.stockprice) || (pvalue < 0.008))
     return(-1);
     vol = 99.5;
     count = 12;
          do
	  {
PBSdiv(vol-0.5,
   status.data[row-8].strike,
   status.stockprice,
   status.interest,
   days,
   &put1,
   &dum);
PBSdiv(vol+0.5,
   status.data[row-8].strike,
   status.stockprice,
   status.interest,
   days,
   &newput,
   &dum);
   slope = newput-put1;
   newput = (newput + put1)*0.5;
		error = fabs(newput-pvalue)/pvalue;
                if (slope < 1e-10)
                  {
                  error = 0;
                  delvol = 1;
                  }
                 else
                  delvol = (pvalue - newput)/slope;
                vol += delvol;
		count--;
          } while ((error > 0.0001)&& (count!= 0));
	  if (count == 0) return(-1);
	  if (fabs(delvol) > 0.1) return(-2);
            return(vol);
}



void calcoverval(int row)
{
if (status.data[row-8].valuec > 0.0)
status.data[row-8].coverval =
(status.data[row-8].marketc - status.data[row-8].valuec) * 100.0 /
  			     (status.data[row-8].valuec);
else status.data[row-8].coverval = 999.99;

if (status.data[row-8].coverval > 999.99)
status.data[row-8].coverval = 999.99;
else
if (status.data[row-8].coverval<-999.99)
status.data[row-8].coverval = -999.99;

if (status.data[row-8].valuep > 0.0)
status.data[row-8].poverval =
(status.data[row-8].marketp - status.data[row-8].valuep) * 100.0 /
  			     (status.data[row-8].valuep);
else status.data[row-8].poverval = 999.99;

if (status.data[row-8].poverval > 999.99)
status.data[row-8].poverval = 999.99;
else
if (status.data[row-8].poverval<-999.99)
status.data[row-8].poverval = -999.99;

 }


void calcalloverval()
{
int row;
for (row=8;row<37;row++)
{
calcoverval(row);
}
}



void calcvolrow(int row)
{
status.data[row-8].volc = invertvolc(row);
status.data[row-8].volp = invertvolp(row);
calcoverval(row);
}


void calcallvol()
{
int row;
for (row=8;row<37;row++)
{
status.data[row-8].volc = invertvolc(row);
status.data[row-8].volp = invertvolp(row);
calcoverval(row);
recalcvolvalues = FALSE;
}
}



