#include <conio.h>
#include <dos.h>
#include <stdlib.h>
#include <math.h>
#include "optdef.h"
#include "options.h"

/* OPTUTIL - UTILITIES FOR OPTIONS ANALYST */


void beep()
{
sound(400);
delay(100);
nosound();
}

void burp()
{
sound(50);
delay(100);
nosound();
}

void waitkey()
{
char ch;
ch = getch();
if (ch == 0) getch();
}


int driveready()
{
int  defaultdrive;
_AX = 0x1900;
geninterrupt(0x21);
defaultdrive = _AL;
if (defaultdrive > 1) return(TRUE);
_AX = 0x0401;
_CX = 0x0101;
_DX = defaultdrive;
geninterrupt(0x13);
if (_AH >= 128)
return(FALSE);
return(TRUE);
}



int dosort(datarow  *mth1,datarow *mth2)
{
int monthint;

monthint = sys.expiry[8].emonth; /* next expiry month */
if	((*mth1).month == -1 && (*mth2).month == -1) return(0);
if	((*mth1).month == -1) return(2);
if	((*mth2).month == -1) return(-2);


if ((*mth1).month == (*mth2).month)
   { if ((*mth1).strike == (*mth2).strike) return(0);
     if ((*mth1).strike < (*mth2).strike) return(-2);
     return(2);
   }

if  ((*mth1).month >= monthint &&  (*mth2).month >= monthint)
       {
	if ((*mth1).month <  (*mth2).month) return(-2);
	return(2);
	}

if ((*mth1).month < monthint && (*mth2).month >= monthint) return(2);

if ((*mth2).month < monthint && (*mth1).month >= monthint) return(-2);

if ((*mth1).month <  (*mth2).month) return(-2);
return(2);
}



void sortstatus()
{
int (*sortrtn)();
	sortrtn = dosort;
	leavecell(sys.cell);
	qsort(&status.data,30,sizeof(datarow),sortrtn);
	entercell(sys.cell);
}


int cursorsave;


void savecursor()
{
_BX = 0;
_AX = 0x0300;
geninterrupt(0x10);
cursorsave = _CX;
};

void retcursor()
{
_CX = cursorsave;
_AX = 0x0100;
geninterrupt(0x10);
};

void hidecursor()
{
_CX = sys.graphics.cursor.hidden;
_AX = 0x0100;
geninterrupt(0x10);
};

void showcursor()
{
_CX = sys.graphics.cursor.shown;
_AX = 0x0100;
geninterrupt(0x10);
};


void goleft()
{
leavecell(sys.cell);
switch (sys.cell.col)
	{
 	 case STRIKE :
	 case MARKETP :
	 case VOLC :
	 case VALUEC :
	 case HELDP :
         case VALUEP : sys.cell.col--; break;
	 case MARKETC :
	 case HELDC : sys.cell.col = STRIKE; break;
         case MONTH : switch (sys.screen)
		      {
                      case SCREEN1 :
			if (sys.display == HELDS)
			sys.cell.col = HELDP;
  		        else
			sys.cell.col = STRIKE;
		        break;
		      case SCREEN3 :
			sys.cell.col = MARKETP;
			break;
		      }
		      break;
	 case SHAREPRICE : sys.cell.col = VOLATILITY; break;
	 case STOCKHELD : sys.cell.col = STOCKHELD; break;
	 case VOLATILITY : sys.cell.col = SHAREPRICE; break;
	 case DATE : sys.cell.col = INTEREST; break;
	 case INTEREST : sys.cell.col = DATE; break;
	 case EXPIRYDAY : sys.cell.col = SHARESPER ; break;
	 case SHARESPER : sys.cell.col = DIVIDENDCENTS; break;
	 case DIVIDENDDAY : sys.cell.col = EXPIRYDAY; break;
	 case DIVIDENDCENTS : sys.cell.col = DIVIDENDDAY; break;
       }
entercell(sys.cell);
}

void goright()
{
leavecell(sys.cell);
switch (sys.cell.col)
	{
 	 case MONTH :
	 case MARKETC :
	 case VALUEC :
	 case HELDC :
         case VALUEP : sys.cell.col++; break;
	 case MARKETP :
	 case HELDP : sys.cell.col = MONTH; break;
         case STRIKE : switch (sys.screen)
			{
			case SCREEN1 :
		        if (sys.display == HELDS)
			sys.cell.col = HELDC;
		        else
			sys.cell.col = MONTH;
			break;
			case SCREEN3 :
			sys.cell.col = MARKETC;
			break;
			}
			break;
	 case SHAREPRICE : sys.cell.col = VOLATILITY; break;
	 case STOCKHELD : sys.cell.col = STOCKHELD; break;
	 case VOLATILITY : sys.cell.col = SHAREPRICE; break;
	 case DATE : sys.cell.col = INTEREST; break;
	 case INTEREST : sys.cell.col = DATE; break;
	 case EXPIRYDAY : sys.cell.col = DIVIDENDDAY ; break;
	 case DIVIDENDDAY : sys.cell.col = DIVIDENDCENTS; break;
	 case DIVIDENDCENTS : sys.cell.col = SHARESPER; break;
	 case SHARESPER : sys.cell.col = EXPIRYDAY;
       }
entercell(sys.cell);
}

void goup()
{
int	pageint;
int	x;
char	stockout[12];
if 	(page == PAGEUP  || sys.screen == SCREEN2 ) pageint = 0;
else	pageint = 15;
leavecell(sys.cell);
if (sys.cell.row > (8 + pageint)) sys.cell.row--;
else
if (sys.cell.row == 8 + pageint)
{
   sys.cell.row = 3;
       switch (sys.cell.col)
       {
 	 case MONTH :
	 case VALUEC :
         case STRIKE :
	 case SHARESPER :
	 case EXPIRYDAY : sys.cell.col = STOCKHELD; sys.cell.row = 4 ; break;
	 case DIVIDENDDAY :
	 case DIVIDENDCENTS :
	 case HELDC :
         case VALUEP :
	 case MARKETC :
	 case MARKETP :
	 case HELDP :  sys.cell.col = VOLATILITY; break;
       }
}
else
switch (sys.cell.row)
{
case 4 :
   sys.cell.row = 3;
   sys.cell.col = SHAREPRICE;
   gotoxy(33,4);
   textbackground(sys.graphics.colors.databack);
   textcolor(sys.graphics.colors.border);
   for ( x = 0; x < 12; x++) putch(HLINE);
   gotoxy(33,4);
	      textcolor(sys.graphics.colors.data);
	      ltoa(status.stockheld, stockout, 10);
	      cprintf("%s ",stockout);
   break;
case 3 :
	 sys.cell.row = 2;
	  switch (sys.cell.col )
	  {
	  case SHAREPRICE : sys.cell.col = DATE ; break;
	  case VOLATILITY : sys.cell.col = INTEREST; break;
	  }
	 break;
case 2 :
       sys.cell.row = 22 + pageint;
       switch (sys.screen)
       {
	case SCREEN1 :
       switch (sys.cell.col)
       {
	 case DATE : sys.cell.col = STRIKE; break;
	 case INTEREST : if (sys.display == HELDS)
			 {sys.cell.col = HELDC; break;}
			 else
			 {sys.cell.col = STRIKE; break;}
       }
       break;
	case SCREEN2 :
       switch (sys.cell.col)
       {
	 case DATE : sys.cell.col = SHARESPER; break;
	 case INTEREST : sys.cell.col = DIVIDENDDAY; break;
       }
       break;
	case SCREEN3 :
       switch (sys.cell.col)
       {
	 case DATE : sys.cell.col = STRIKE; break;
	 case INTEREST : sys.cell.col = MARKETC; break;
       }
       break;
	}
break;
}
entercell(sys.cell);
}


void godown()
{
int	pageint;
int	x;
char	stockout[12];
if 	(page == PAGEUP || sys.screen == SCREEN2) pageint = 0;
else	pageint = 15;
leavecell(sys.cell);
if ((sys.cell.row < 22+pageint) && (sys.cell.row > 7+ pageint)) sys.cell.row++;
else
if (sys.cell.row == 22+pageint)
switch (sys.cell.col)
{
case MONTH :
case STRIKE :
case EXPIRYDAY :
case SHARESPER : sys.cell.row = 2;sys.cell.col = DATE; break;
case HELDC :
case HELDP :
case DELTAC :
case DELTAP :
case DIVIDENDDAY :
case DIVIDENDCENTS :
case MARKETC :
case MARKETP : sys.cell.row = 2;sys.cell.col = INTEREST; break;
}
else
switch (sys.cell.row)
{
case 2 :
	sys.cell.row = 3;
	switch (sys.cell.col)
	{
	 case  DATE : sys.cell.col = SHAREPRICE; break;
	 case  INTEREST : sys.cell.col = VOLATILITY; break;
	 }
	 break;
case 3 : switch (sys.cell.col)
	 {
	 case SHAREPRICE : sys.cell.row = 4;sys.cell.col = STOCKHELD; break;
	 case VOLATILITY : sys.cell.row = 8+pageint;
			 switch (sys.screen)
			 {
			 case SCREEN1 :
			  if (sys.display == HELDS)
			  sys.cell.col = HELDC;
			  else
			  sys.cell.col = STRIKE;
			  break;
			 case SCREEN2 :
			  sys.cell.col = DIVIDENDDAY;
			  break;
			 case SCREEN3 :
			  sys.cell.col = MARKETC;
			  break;
			 }
			 break;
	 }
	 break;
case 4 : sys.cell.row = 8+pageint;
	 switch (sys.screen)
	 {
	 case SCREEN3 :
	 case SCREEN1 :
	  sys.cell.col = STRIKE;
	  break;
	 case SCREEN2 :
	  sys.cell.col = SHARESPER;
	  break;
	 }
}
entercell(sys.cell);
}










int serialdate(int year,int month,int day)
{
static int monthdays[12] =  {0,31,59,90,120,151,181,212,243,273,304,334};
int sdate;
  sdate = 365 * (year-40) + (year + 3)/4;
  if (4* (int) (year/4) == year && month  > 1)
  sdate = sdate + 1;
  return(sdate + monthdays[month] + day);
}


void convertdate(char date[7],int *year,int *month,int *day)
{
char dumstr[3];
dumstr[0] = date[0];
dumstr[1] = date[1];
dumstr[2] = 0;
*day = atoi(dumstr);
dumstr[0] = date[5];
dumstr[1] = date[6];
dumstr[2] = 0;
*year = atoi(dumstr);
dumstr[0] = date[2];
dumstr[1] = date[3];
dumstr[2] = date[4];
dumstr[3] = 0;
*month = findmonth(dumstr);
}





void clearall()
{
	int 	x;
	for	( x=0;x < 30; x++)
	{
		status.data[x].month = -1;
		status.data[x].strike = 0;
		status.data[x].heldc = 0;
		status.data[x].heldp = 0;
		status.data[x].marketc = 0;
		status.data[x].marketp = 0;
	}
	for	(x = 0; x < 24; x++)
		{
		status.sizepay[x].dday = 0;
		status.sizepay[x].payout = 0;
                status.sizepay[x].sharesper = 1000;
		}
	status.stockheld = 0;
}




void calcdays()
{
int row;
int y1,m1,m2,d1;
convertdate(sys.date,&y1,&m1,&d1);
for (row=8;row<=22;row++)
sys.expiry[row].daysleft = serialdate(sys.expiry[row].eyear, sys.expiry[row].emonth,
                        sys.expiry[row].eday) - serialdate(y1, m1, d1);
}



void filldates()
{
int   count;
 if (sys.expiry[8].eyear == 0)
  {
  sys.expiry[8].eyear = syear;
  sys.expiry[8].emonth= smonth;
  if (sday > 28) sys.expiry[8].eday = sday;
  else sys.expiry[8].eday = 28;
  }
  for (count = 9; count <= 23; count++)
   {
   if ( sys.expiry[count].eyear == 0 || !(( sys.expiry[count -1].eyear
       == sys.expiry[count].eyear && sys.expiry[count-1].emonth ==
       sys.expiry[count].emonth -1) || (sys.expiry[count-1].eyear +1 ==
       sys.expiry[count].eyear && sys.expiry[count-1].emonth == 11
       && sys.expiry[count].emonth == 0)))
       {
       sys.expiry[count].emonth = sys.expiry[count-1].emonth +1;
       if (sys.expiry[count].emonth == 12)
	 { sys.expiry[count].emonth = 0;
	   sys.expiry[count].eyear = sys.expiry[count-1].eyear +1;
	   }
	 else sys.expiry[count].eyear = sys.expiry[count-1].eyear ;
	 sys.expiry[count].eday = 28;
     }
   }    /* for 9 t0 23*/
   for (count =7; count >= 0; count--)
     {
      if ( sys.expiry[count].eyear == 0 || !(( sys.expiry[count].eyear ==
      sys.expiry[count+1].eyear && sys.expiry[count].emonth == sys.expiry[count +1].emonth-1) ||
      ( sys.expiry[count].eyear +1 == sys.expiry[count + 1].eyear && sys.expiry[count].emonth == 11
       && sys.expiry[count +1].emonth == 0)))
       {
       sys.expiry[count].emonth = sys.expiry[count +1].emonth -1;
       if (sys.expiry[count].emonth == -1 )
	{
	 sys.expiry[count].emonth = 11;
	 sys.expiry[count].eyear = sys.expiry[count +1].eyear -1;
	 }
	 else sys.expiry[count].eyear = sys.expiry[count + 1].eyear;
	 sys.expiry[count].eday = 28;
       }
      }
}


void rollexup(int n)
{
int moveit;
for (moveit = 0; moveit < 24-n; moveit++)
 sys.expiry[moveit] = sys.expiry[moveit + n];
for (moveit = 24-n;moveit < 24; moveit++)
{
 sys.expiry[moveit].eyear = 0;
 sys.expiry[moveit].emonth = 0;
}
}

void rollexdown(int n)
{
int moveit;
for (moveit = 23; moveit >= n; moveit--)
sys.expiry[moveit] = sys.expiry[moveit - n];
for (moveit = n-1;moveit >= 0;moveit--)
{
sys.expiry[moveit].eyear = 0;
sys.expiry[moveit].emonth = 0;
}
}




void initdates()
{
int shift;
convertdate(sys.date,&syear,&smonth,&sday);

if ((sys.expiry[8].eyear < syear) || (sys.expiry[8].eyear == syear &&
    sys.expiry[8].emonth < smonth) || (sys.expiry[8].eyear == syear &&
    sys.expiry[8].emonth == smonth && sys.expiry[8].eday < sday))
    {
    shift = (syear-sys.expiry[8].eyear)*12 + (smonth-sys.expiry[8].emonth);
    if (shift > 20)
    sys.expiry[8].eyear = 0;
    else
    rollexup(shift);
    }
 else
    {
    shift = (sys.expiry[8].eyear-syear)*12 + (sys.expiry[8].emonth-smonth);
    if (shift > 20)
    sys.expiry[8].eyear = 0;
    else
    rollexdown(shift);
    }
filldates();
}



void rollpayup(int n)
{
int moveit;
for (moveit = 0; moveit < 24-n; moveit++)
 status.sizepay[moveit] = status.sizepay[moveit + n];
for (moveit = 24-n;moveit < 24; moveit++)
{
 status.sizepay[moveit].dyear = 0;
 status.sizepay[moveit].dmonth = 0;
     status.sizepay[moveit].dday = 0;
     status.sizepay[moveit].payout = 0;
}
}

void rollpaydown(int n)
{
int moveit;
for (moveit = 23; moveit >= n; moveit--)
status.sizepay[moveit] = status.sizepay[moveit - n];
for (moveit = n-1;moveit >= 0;moveit--)
{
status.sizepay[moveit].dyear = 0;
status.sizepay[moveit].dmonth = 0;
     status.sizepay[moveit].dday = 0;
     status.sizepay[moveit].payout = 0;
}
}



void fillpay()
{
int count;
if ( status.sizepay[8].dyear == 0 ||
   status.sizepay[8].dmonth != sys.expiry[8].emonth || status.sizepay[8].dyear
   != sys.expiry[8].eyear)
    for (count = 0; count <= 23; count++)
    { status.sizepay[count].dyear = sys.expiry[count].eyear;
     status.sizepay[count].dmonth = sys.expiry[count].emonth;
     status.sizepay[count].dday = 0;
     status.sizepay[count].payout = 0;
     status.sizepay[count].sharesper = 1000;
     }
     else
     {
      for (count = 9; count <=23; count++)
	{
	if (!( (status.sizepay[count].dmonth == status.sizepay[count-1].dmonth +1 &&
	     status.sizepay[count-1].dyear == status.sizepay[count].dyear)||
	   ( status.sizepay[count].dyear == status.sizepay[count-1].dyear +1 &&
	status.sizepay[count-1].dmonth == 11 && status.sizepay[count].dyear == 0)))
	 {
	     status.sizepay[count].dyear = sys.expiry[count].eyear;
	     status.sizepay[count].dmonth = sys.expiry[count].emonth;
	     status.sizepay[count].dday = 0;
	     status.sizepay[count].payout = 0;
	     status.sizepay[count].sharesper = 1000;
	 }
       }
       for (count = 7; count >= 0; count--)
	{
	if (!( (status.sizepay[count+ 1].dmonth == status.sizepay[count].dmonth +1 &&
	     status.sizepay[count].dyear == status.sizepay[count+1].dyear ) ||
	   ( status.sizepay[count+1].dyear == status.sizepay[count].dyear +1 &&
	status.sizepay[count].dmonth == 11 && status.sizepay[count+1].dyear ==0)))
	 {
	     status.sizepay[count].dyear = sys.expiry[count].eyear;
	     status.sizepay[count].dmonth = sys.expiry[count].emonth;
	     status.sizepay[count].dday = 0;
	     status.sizepay[count].payout = 0;
	     status.sizepay[count].sharesper = 1000;
	 }
       }
    }
}



void initsizepay()
{
int x,shift;

if (status.sizepay[8].dyear >  sys.expiry[8].eyear ||
      (status.sizepay[8].dyear ==  sys.expiry[8].eyear &&
       status.sizepay[8].dmonth >  sys.expiry[8].emonth))
    {
    shift = (status.sizepay[8].dyear-sys.expiry[8].eyear)*12 +
    (status.sizepay[8].dmonth - sys.expiry[8].emonth);
    if (shift > 20)
    status.sizepay[8].dyear = 0;
    else
    rollpaydown(shift);
    }
else
    {
    shift = (sys.expiry[8].eyear-status.sizepay[8].dyear)*12
    + (sys.expiry[8].emonth - status.sizepay[8].dmonth);
    if (shift > 20)
    status.sizepay[8].dyear = 0;
    else
    rollpayup(shift);
    }
fillpay();
}





int dtx(int month)
{
int x;
x = month - sys.expiry[8].emonth;
if (x < 0) x += 12;
return( sys.expiry[x+8].daysleft);
}

int shmultiplier(int month)
{
int x;
x = month - status.sizepay[8].dmonth;
if (x < 0) x += 12;
return(status.sizepay[x +8].sharesper);
}


int mindtx()
{
int	x;
int	y;
int	z;
int	done;
done = 0;
x = sys.expiry[8].emonth;
while (done != 12)
{
	for (y = 0; y < 30; y++)
	{
	  if (x == status.data[y].month &&
	  (status.data[y].heldc != 0 || status.data[y].heldp != 0))
	     return( sys.expiry[8 +done].daysleft);
	 }
	  done++;
	  x++;
	  if (x == 12) x = 0;
}
return(0);
/* return 0 if no months found */
}



int maxdtx()
{
int	x;
int	y;
int	z;
int	done;
done = 10;
x = sys.expiry[8+10].emonth;
while (done != 0)
{
	for (y = 0; y < 30; y++)
	{
	  if (x == status.data[y].month &&
	  (status.data[y].heldc != 0 || status.data[y].heldp != 0))
	     return( sys.expiry[8 +done].daysleft);
	 }
	  done--;
	  x--;
	  if (x == -1) x = 11;
}
return(0);
/* return 0 if no months found */
}



void totals()
{
int count;
double totalweight,weight,totalweightc,totalweightp;
totalvalue = totaldelta = totalcalls= totalputs= 0;

for (count = 0;count < 30;count++)
if (status.data[count].month != -1)
{
totalvalue += shmultiplier(status.data[count].month)
		* (status.data[count].valuec * status.data[count].heldc
		  +status.data[count].valuep * status.data[count].heldp);
totaldelta += shmultiplier(status.data[count].month)
		* (status.data[count].deltac * status.data[count].heldc
		  +status.data[count].deltap * status.data[count].heldp);
totalcalls += status.data[count].heldc;
totalputs += status.data[count].heldp;
}
totalvalue +=  status.stockheld * status.stockprice;
totaldelta = totaldelta + status.stockheld;

totalweight = weightedvol = weightedvolc = weightedvolp = 0.0;
totalweightc = totalweightp = 0.0;

if (status.stockprice >= 0.01)
for (count = 0;count < 30;count++)
{
weight = 0.7 - fabs(status.stockprice-status.data[count].strike)/status.stockprice;
if (weight < 0) weight = 0.0;

if (status.data[count].month >= 0 && status.data[count].volc > 0)
	{
        totalweightc += weight;
        totalweight += weight;
	weightedvolc += weight*status.data[count].volc;
	weightedvol += weight*status.data[count].volc;
	}
if (status.data[count].month >= 0 && status.data[count].volp > 0)
	{
	totalweightp += weight;
        totalweight += weight;
	weightedvolp += weight*status.data[count].volp;
	weightedvol += weight*status.data[count].volp;
	}
}
if (totalweight > 0.0)
	{
	weightedvol = weightedvol/totalweight;
	if (weightedvolp > 0.0)
	weightedvolp = weightedvolp/totalweightp;
	else
	weightedvolp = 0.0;
	if (weightedvolc > 0.0)
	weightedvolc = weightedvolc/totalweightc;
	else
	weightedvolc = 0.0;
	}
	else weightedvol = weightedvolp = weightedvolc = 0.0;
}


