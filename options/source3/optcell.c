
#include <conio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "optdef.h"
#include "options.h"

int startcell(enum coltype col)
{   switch (col)
       {case MONTH : return(2);
	case STRIKE : return(14);
	case COVERVAL :
	case VOLC :
	case VALUEC : return(29);
	case POVERVAL :
	case VOLP :
	case VALUEP : return(42);
	case MARKETC :
	case DELTAC :
	case HELDC : return(55);
	case MARKETP :
	case DELTAP:
	case HELDP : return(68);
	case SHAREPRICE : return(31);
	case STOCKHELD : return(31);
	case VOLATILITY : return(67);
	case INTEREST   : return(67);
	case  DATE      : return(31);
	case YEARMONTH : return(2);
	case SHARESPER : return(14);
	case EXPIRYDAY : return(29);
	case DAYSLEFT : return(42);
	case DIVIDENDDAY : return(55);
	case DIVIDENDCENTS : return(68);
       }
}

int endcell(enum coltype col)
{   switch (col)
       {case MONTH : return(12);
	case STRIKE : return(27);
	case COVERVAL :
	case VOLC :
	case VALUEC : return(40);
	case POVERVAL :
	case VOLP :
	case VALUEP : return(53);
	case MARKETC :
	case DELTAC :
	case HELDC : return(66);
	case MARKETP :
	case DELTAP :
	case HELDP : return(79);
	case SHAREPRICE : return(41);
	case STOCKHELD : return(41);
	case VOLATILITY : return(70);
	case INTEREST   : return(70);
	case DATE    : return(41);
	case YEARMONTH : return(12);
	case SHARESPER : return(27);
	case EXPIRYDAY : return(40);
	case DAYSLEFT : return(53);
	case DIVIDENDDAY : return(66);
	case DIVIDENDCENTS : return(79);
       }
}


void blankcell(celltype cell)
{
int	pageint;
if (page == PAGEUP || sys.screen == SCREEN2 || cell.row < 8) pageint = 0;
else pageint = 15;
window(startcell(cell.col),cell.row-pageint,endcell(cell.col),cell.row- pageint);
clrscr();
window(1,1,80,25);
}

void blankrow(int row)
{
int	x;
int	pageint;
if (page == PAGEUP || sys.screen == SCREEN2 || row < 8) pageint = 0;
else pageint = 15;

 textbackground(sys.graphics.colors.databack);
for   ( x = MONTH ; x <= HELDP; x++)
   {
	window(startcell(x),row-pageint, endcell(x),row-pageint);
	clrscr();
   }
	window(1,1,80,25);
}


void blankall()
{
int 	x;
 textbackground(sys.graphics.colors.databack);
for   ( x= MONTH ; x <= HELDP; x++)
   {
	window(startcell(x),8, endcell(x),22);
	clrscr();
   }
	window(1,1,80,25);
}


void showcell(celltype cell)
{
char outstring[13];
int pageint;
int	x;
if (page == PAGEUP || sys.screen == SCREEN2 || cell.row < 8 ) pageint = 0;
else pageint = 15;
outstring[0] = 0;
switch (cell.col)

	{

	case MONTH  :if (cell.row == 8 + pageint ||
	  status.data[cell.row-8 ].month != status.data[cell.row-9].month)
		     {
		     strncpy(outstring,months[status.data[cell.row-8].month],3);
		     outstring[3] = 0;
		     strcat(outstring,"   ");
		     }
		     else strcpy(outstring,"       ");
		     break;
	case STRIKE : sprintf(outstring,"%8.*f",sys.decimal,status.data[cell.row-8 ].strike); break;
	case VALUEC : sprintf(outstring,"%8.*f",sys.decimal,status.data[cell.row-8 ].valuec); break;
	case VALUEP : sprintf(outstring,"%8.*f",sys.decimal,status.data[cell.row-8 ].valuep); break;
	case HELDC : if (status.data[cell.row-8].heldc == 0)
		     strcpy(outstring," ");
		     else
		     sprintf(outstring,"%d",status.data[cell.row-8 ].heldc);
		     break;
	case HELDP : if (status.data[cell.row-8].heldp == 0)
		     strcpy(outstring," ");
		     else
                     sprintf(outstring,"%d",status.data[cell.row-8 ].heldp);
		     break;
	case DELTAC : sprintf(outstring,"%8.*f",sys.decimal,status.data[cell.row-8 ].deltac); break;
	case DELTAP : sprintf(outstring,"%8.*f",sys.decimal,status.data[cell.row-8 ].deltap); break;
	case SHAREPRICE :  sprintf(outstring,"$%-5.2f",status.stockprice);
			gotoxy(33,3); cprintf("%s",outstring); break;
	case STOCKHELD : gotoxy(33,4); ltoa(status.stockheld, outstring, 10);
			 cprintf("%s ", outstring);
			 textcolor(sys.graphics.colors.border);
			 textbackground(sys.graphics.colors.databack);
			 textlineh(4,wherex(),45);
			 textcolor(sys.graphics.colors.data);
			 break;
	case VOLATILITY :  sprintf(outstring,"%-5.2f",status.volatility); break;
	case INTEREST   :  sprintf(outstring,"%-5.2f",status.interest); break;
	case DATE :  strncpy(outstring,sys.date,7);
		     outstring[7] = 0;
		     break;
	case YEARMONTH :
                        if (cell.row == 8 ||

			sys.expiry[cell.row].eyear != sys.expiry[cell.row-1].eyear)
			{
			if (sys.expiry[cell.row].eyear < 40)
			sprintf(outstring,"20%-d ",sys.expiry[cell.row].eyear);
			else
			sprintf(outstring,"19%-d ",sys.expiry[cell.row].eyear);
                        }
			strncat(outstring,months[sys.expiry[cell.row].emonth],3);
			outstring[8] = 0;
			break;
	case SHARESPER : sprintf(outstring,"%d",status.sizepay[cell.row].sharesper); break;
	case EXPIRYDAY :  sprintf(outstring,"%d",sys.expiry[cell.row].eday); break;
	case DAYSLEFT : if (sys.expiry[cell.row].daysleft < 0)
			strcpy(outstring," ");
			else
			sprintf(outstring,"%d",sys.expiry[cell.row].daysleft);
			break;
	case DIVIDENDDAY : if (status.sizepay[cell.row].dday == 0)
			   strcpy(outstring," ");
			   else
                           sprintf(outstring,"%d",status.sizepay[cell.row].dday);
			   break;
	case DIVIDENDCENTS :if (status.sizepay[cell.row].payout == 0)
			    strcpy(outstring," ");
			    else
			    sprintf(outstring,"%d",status.sizepay[cell.row].payout);
			    break;
	case VOLC :     if (status.data[cell.row-8].marketc == 0.0)
			strcpy(outstring," ");
			else
			if (status.data[cell.row-8].volc <= 0.0)
			strcpy(outstring,"*");
			else
			sprintf(outstring,"%8.*f",1,status.data[cell.row-8 ].volc);
			break;
	case VOLP :     if (status.data[cell.row-8].marketp == 0.0)
			strcpy(outstring," ");
			else
			if (status.data[cell.row-8].volp <= 0.0)
			strcpy(outstring,"*");
			else
			sprintf(outstring,"%8.*f",1,status.data[cell.row-8 ].volp);
			break;
	case MARKETC :     if (status.data[cell.row-8].marketc == 0.0)
			strcpy(outstring," ");
			else
			sprintf(outstring,"%8.*f",sys.decimal,status.data[cell.row-8 ].marketc);
			break;
	case MARKETP :     if (status.data[cell.row-8].marketp == 0.0)
			strcpy(outstring," ");
			else
			sprintf(outstring,"%8.*f",sys.decimal,status.data[cell.row-8 ].marketp);
			break;
	case COVERVAL : if (status.data[cell.row-8].marketc == 0.0)
                        strcpy(outstring," ");
			else
			if (status.data[cell.row-8].coverval >= 999.99)
			sprintf(outstring,">%8.*f",1,status.data[cell.row-8].coverval);
			else
			if (status.data[cell.row-8].coverval <= -999.99)
			sprintf(outstring,"<%8.*f",1,status.data[cell.row-8].coverval);
			else
			sprintf(outstring,"%8.*f",1,status.data[cell.row-8 ].coverval);
			break;
	case POVERVAL : if (status.data[cell.row-8].marketp == 0.0)
                        strcpy(outstring," ");
			else
			 if (status.data[cell.row-8].poverval >= 999.99)
			sprintf(outstring,">%8.*f",1,status.data[cell.row-8].poverval);
			else
			if (status.data[cell.row-8].poverval <= -999.99)
			sprintf(outstring,"<%8.*f",1,status.data[cell.row-8].poverval);
			else
			sprintf(outstring,"%8.*f",1,status.data[cell.row-8 ].poverval);
			break;

        }
	if ((sys.screen == SCREEN1 || sys.screen == SCREEN3)
	&& cell.row >= 8
	&& status.data[cell.row-8 ].month < 0)
	strcpy(outstring,"     ");
gotoxy(endcell(cell.col)-1-strlen(outstring),cell.row- pageint);
if (cell.col != STOCKHELD && cell.col != SHAREPRICE)
cprintf(outstring);
}


void entercell(celltype cell)
{
char sharestrin[12];
textcolor(sys.graphics.colors.highback);
textbackground(sys.graphics.colors.databack);
if (cell.row > 7 && cell.row < 23)
{
gotoxy(1,cell.row);
putch(16);
gotoxy(80,cell.row);
putch(17);
}
else
if (cell.row > 22)
{
gotoxy(1,cell.row-15);
putch(16);
gotoxy(80,cell.row-15);
putch(17);
}
textcolor(sys.graphics.colors.highlight);
textbackground(sys.graphics.colors.highback);
blankcell(cell);
if (cell.col != STOCKHELD)
showcell(cell);
else
   {
    gotoxy(33,4); ltoa(status.stockheld, sharestrin, 10);
    cprintf("%s ", sharestrin);
    while (wherex() <= endcell(STOCKHELD)) putch(' ');
   }
}


void leavecell(celltype cell)
{
textcolor(sys.graphics.colors.border);
textbackground(sys.graphics.colors.databack);
if (cell.row > 7 && cell.row < 23)
{
gotoxy(1,cell.row);
putch(VLINE);
gotoxy(80,cell.row);
putch(VLINE);
}
else
if (cell.row > 22)
{
gotoxy(1,cell.row-15);
putch(VLINE);
gotoxy(80,cell.row-15);
putch(VLINE);
}
textcolor(sys.graphics.colors.data);
textbackground(sys.graphics.colors.databack);
blankcell(cell);
showcell(cell);
}
