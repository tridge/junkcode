#include <fcntl.h>
#include <stat.h>
#include <io.h>
#include <dos.h>
#include <stdlib.h>
#include <dir.h>
#include <string.h>
#include <stdio.h>
#include <bios.h>
#include <conio.h>
#include "optdef.h"
#include "options.h"

/* OPTOUT - OUTPUT FUNCTIONS FOR OPTIONS ANALYST */


void copyright(void)
{
char *text =
"\n"
"\n"
"                           The\n\n"
"                     OPTIONS ANALYST\n"
"\n\n\n\n"
"                    Copyright (c) 1988\n"
"                 EFAM RESOURCES Pty. Ltd.\n"
"                   All Rights Reserved\n\n"
"  Press Any Key";
showhelp("Version 1.0",text);
}


void wait()
{
	window(1,1,80,25);
	gotoxy(73,1);
	textcolor(sys.graphics.colors.databack + 128);
	textbackground(sys.graphics.colors.heading2);
	cprintf("  WAIT ");
	textcolor(sys.graphics.colors.data);
	textbackground(sys.graphics.colors.databack);
}

void ready()
{
	window(1,1,80,25);
	gotoxy(73,1);
	textcolor(sys.graphics.colors.databack);
	textbackground(sys.graphics.colors.heading2);
	cprintf(" READY ");
	textcolor(sys.graphics.colors.data);
	textbackground(sys.graphics.colors.databack);
}


void writefile(char *fname)
 {
char error[60];

int fhandle;
if (driveready() == TRUE)
{
	fhandle = open(fname, O_RDWR | O_CREAT | O_TRUNC | O_BINARY,
                              S_IREAD | S_IWRITE);
	if (fhandle != -1)
	{
        if (write(fhandle,&status,sizeof(statustype)) < sizeof(statustype))
	errormessage("Disk access error\nDisk probably full");
	else
        strncpy(sys.lastfile,fname,15);
	close(fhandle);
	}
	else
	{
	sprintf(error,"Disk access error\n%s",sys_errlist[errno]);
	errormessage(error);
	}
} else errormessage("Drive not ready 11\nCheck hardware");
strncpy(filename,sys.lastfile,15);
}


void textlinev(int col, int start, int finish, int topch, int botch)
{
int count;

	for(count = start+1; count < finish; count++)
	{
	gotoxy(col,count);
	putch(VLINE);
	}
	gotoxy(col,start);
	putch(topch);
	gotoxy(col,finish);
	putch(botch);
}

void textlineh(int row, int start, int finish)
{
int count;

	for(count = start; count <= finish; count++)
	{
	gotoxy(count,row);
	putch(HLINE);
	}
}

void showrow(int row)
{
celltype cell;
cell.row = row;
blankrow(row);
textcolor(sys.graphics.colors.data);
textbackground(sys.graphics.colors.databack);

switch (sys.screen)
{
case SCREEN1 :
	for(cell.col=MONTH;cell.col<=VALUEP;cell.col++)
	showcell(cell);
	if (sys.display == HELDS)
	{
	cell.col = HELDC;
	showcell(cell);
	cell.col = HELDP;
	showcell(cell);
	}
	else
	{
	cell.col = DELTAC;
	showcell(cell);
	cell.col = DELTAP;
	showcell(cell);
	}
	break;
case SCREEN2 :
	for (cell.col=YEARMONTH;cell.col<=DIVIDENDCENTS;cell.col++)
	showcell(cell);
	break;
case SCREEN3 :
	for (cell.col=MONTH;cell.col<=STRIKE;cell.col++)
	showcell(cell);
	for (cell.col=MARKETC;cell.col<=MARKETP;cell.col++)
	showcell(cell);
	if (sys.display == INVVOL)
	  {	for (cell.col=VOLC;cell.col<=VOLP;cell.col++)
              showcell(cell);
	  }
	else
	if (sys.display == OVERVALUED)
	  {	for (cell.col=COVERVAL;cell.col<=POVERVAL;cell.col++)
              showcell(cell);
	  }
	break;
}
}


void showall()
{
int	pageint;
celltype cell;
if (page == PAGEUP || sys.screen == SCREEN2) pageint = 0;
else pageint = 15;
blankall();
textcolor(sys.graphics.colors.data);
textbackground(sys.graphics.colors.databack);
switch (sys.screen)
{
case SCREEN1 :
      for(cell.row=8+pageint ;cell.row<=22 +pageint ;cell.row++)
	{
	for(cell.col=MONTH;cell.col<=VALUEP;cell.col++)
	showcell(cell);
	if (sys.display == HELDS)
		{
		cell.col = HELDC;
		showcell(cell);
		cell.col = HELDP;
		showcell(cell);
		}
	else
		{
		cell.col = DELTAC;
		showcell(cell);
		cell.col = DELTAP;
		showcell(cell);
		}
	}
	break;
case SCREEN2 :
	for(cell.row=8;cell.row<=22;cell.row++)
	for(cell.col=YEARMONTH;cell.col<=DIVIDENDCENTS;cell.col++)
	showcell(cell);
	break;
case SCREEN3 :
      for(cell.row=8+pageint ;cell.row<=22 +pageint ;cell.row++)
	{
	for (cell.col=MONTH;cell.col<=STRIKE;cell.col++)
	showcell(cell);
	for (cell.col=MARKETC;cell.col<=MARKETP;cell.col++)
	showcell(cell);
	if (sys.display == INVVOL)
	  {	for (cell.col=VOLC;cell.col<=VOLP;cell.col++)
              showcell(cell);
	  }
	else
	if (sys.display == OVERVALUED)
	  {	for (cell.col=COVERVAL;cell.col<=POVERVAL;cell.col++)
              showcell(cell);
	  }
	}
	break;
}
cell.row=2;
cell.col = DATE;
showcell(cell);
cell.col = INTEREST;
showcell(cell);
cell.row=3;
cell.col =SHAREPRICE;
showcell(cell);
cell.col = VOLATILITY;
showcell(cell);
cell.col = STOCKHELD;
cell.row = 4;
showcell(cell);
wait();
ready();
}


void drawborder()
{
textcolor(sys.graphics.colors.border);
textlineh(4,2,79);
textlineh(7,2,79);
textlineh(23,2,79);
textlinev(1,4,6,218,VLINE);
textlinev(80,4,6,191,VLINE);
textlinev(1,7,23,195,192);
textlinev(13,7,23,194,193);
textlinev(28,7,23,194,193);
textlinev(41,7,23,194,193);
textlinev(54,7,23,194,193);
textlinev(67,7,23,194,193);
textlinev(80,7,23,180,217);
gotoxy(21,4);
textcolor(sys.graphics.colors.status);
cprintf(" STOCK =    ");

}  /* drawborder */


void disppage()
{
textcolor(sys.graphics.colors.heading2);
textbackground(sys.graphics.colors.databack);
gotoxy(3,4);
if (page == PAGEUP)
cprintf("Page 1");
else
cprintf("Page 2");
}



void showtotals()
{
char headstr[30];
char numstr1[30];
char numstr2[30];
char numstr3[30];
char endstring[30];
coltype col1,col2;
int x;
double y;

if (totalvalue >= 1.0e8 || totalvalue <= -1.0e8)
{
burp();
if (totalvalue > 0.0)
{
makewindow(10,5,60,19,"TOO RICH");
gotoxy(14,8);
cprintf("You are too Rich.");
gotoxy(14,10);
cprintf("This is a free society so you have a choice:");
gotoxy(14,12);
cprintf("1. Make a LARGE DONATION TO CHARITY.");
gotoxy(14,14);
cprintf("2. Buy an EXPENSIVE version of this program.");
gotoxy(14,16);
}
else
{
makewindow(10,5,60,19,"BANKRUPT !");
gotoxy(14,8);
cprintf("You are too Poor.");
gotoxy(14,10);
cprintf("This is a free society so you have a choice:");
gotoxy(14,12);
cprintf("1. Become a charity.");
gotoxy(14,14);
cprintf("2. Buy an EXPENSIVE version of this program.");
gotoxy(14,16);
}
cprintf("NOTE: We cannot guarantee values produced");
gotoxy(14,18);
cprintf("      when such large numbers are used.");
delay(6000);
closewindow(10,5,60,19);
totalvalue = 0.0;
}

switch (sys.screen)
{
case SCREEN1 :
		ltoa(totalputs,numstr3,10);
		ltoa(totalcalls,numstr2,10);
		col1 = HELDC;
		col2 = HELDP;
		if (sys.display == HELDS)
                {
		strcpy(headstr," Tot. Value $  ");
		sprintf(numstr1,"%-12.2f",totalvalue);
		strcpy(endstring," ");
		}
		else
		{
		strcpy(headstr," Tot. Delta    ");
		sprintf(numstr1,"%-12.2f",totaldelta);
		strcpy(endstring,"$/c");
		}
		break;


case SCREEN2 :  break;
case SCREEN3 : if (sys.display == INVVOL || sys.display == OVERVALUED)
		{
		strcpy(headstr," Weighted Vol. ");
		strcpy(endstring," \% ");
		sprintf(numstr1,"%-5.*f",2,weightedvol);
		sprintf(numstr2,"%-4.*f \%",2,weightedvolc);
		sprintf(numstr3,"%-4.*f \%",2,weightedvolp);
		col1 = VOLC;
		col2 = VOLP;
		}
		break;
}

if (sys.screen != SCREEN2)
{
textcolor(sys.graphics.colors.border);
textbackground(sys.graphics.colors.databack);
textlineh(23,startcell(HELDC),endcell(HELDC));
textlineh(23,startcell(HELDP),endcell(HELDP));
textlineh(23,startcell(VOLC),endcell(VOLC));
textlineh(23,startcell(VOLP),endcell(VOLP));

textcolor(sys.graphics.colors.heading2);
  x = strlen(numstr2)/2;
  gotoxy((startcell(col1)+endcell(col1))/2 - x, 23);
  cprintf(" %s ",numstr2);

  x = strlen(numstr3)/2;
  gotoxy((startcell(col2)+endcell(col2))/2 - x, 23);
  cprintf(" %s ",numstr3);

gotoxy(49,4);

for (x = 0; x < 12; x++)
if (numstr1[x] <= ' ') numstr1[x] = 0;
numstr1[12] = 0;
cprintf("%s%s%s",headstr,numstr1,endstring);
textcolor(sys.graphics.colors.border);
textlineh(4,wherex(),79);
}
}







int printdetails()
{
int x;
int errorc;
char lineout[80];
char linebuf[45];
char linebuf2[40];
int lineposn;
int count;
if (biosprint(2,0,0) & 1 == 1)
{
errormessage("Printer Error\nCheck Hardware");
return(0);
}
errorc = 0;
errorc = 41 & biosprint(1,0,0);
errorc = 41 & biosprint(0,13,0);
errorc = 41 & biosprint(0,10,0);
lineposn = sprintf(lineout, "FILE : %s  ", sys.lastfile);
for (x = lineposn; x < 30; x++)
lineout[x-1] = ' ';
lineout[25] = 0;
lineout[23] = 32;
strcpy(linebuf2,sys.date);
lineposn = 24 + sprintf(linebuf,"DATE : %s  ", linebuf2);
strcat(lineout,linebuf);
for (x = lineposn; x < 59; x++)
lineout[x-1] = ' ';
lineout[50] = 0;
lineposn = 49 + sprintf(linebuf,"INTEREST RATE : %-5.2f \%   ", status.interest);
strcat(lineout,linebuf);
for (x = lineposn; x < 86; x++)
lineout[x-1] = ' ';
lineout[76] = 13;
lineout[24] = ' ';
lineout[49] = ' ';
lineout[77] = 10;
for (x = 0; x <= 77; x++)
if (errorc == 0)
errorc = 41 & biosprint(0,lineout[x],0);
ltoa(totalcalls, linebuf2, 10);
lineposn = sprintf(lineout, "TOT. No. CALLS = %s  ", linebuf2);
for (x = lineposn; x < 29; x++)
lineout[x-1] = ' ';
lineout[25] = 0;
lineposn = 24 + sprintf(linebuf,"TOT. VALUE $ %-10.2f  ", totalvalue);
strcat(lineout,linebuf);
for (x = lineposn; x < 59; x++)
lineout[x-1] = ' ';
lineout[50] = 0;
lineposn = 49 + sprintf(linebuf,"VOLATILITY : %-5.2f \%             ", status.volatility);
strcat(lineout,linebuf);
for (x = 73; x < 76; x++)
lineout[x-1] = ' ';
lineout[76] = 13;
lineout[77] = 10;
lineout[24] = ' ';
for (x = 0; x <= 77; x++)
if (errorc == 0)
errorc = 41 & biosprint(0,lineout[x],0);
ltoa(totalputs, linebuf2, 10);
lineposn = sprintf(lineout, "TOT. No. PUTS  = %s  ", linebuf2);
for (x = lineposn; x < 29; x++)
lineout[x-1] = ' ';
lineout[25] = 0;
lineposn = 24 +  sprintf(linebuf,"TOT. DELTA %-7.2f $/c            ", totaldelta* 0.01);
strcat(lineout,linebuf);
for (x = lineposn; x < 60; x++)
lineout[x-1] = ' ';
lineout[50] = 0;
lineposn = 49 + sprintf(linebuf,"SHARE PRICE  $%-5.2f  ", status.stockprice);
strcat(lineout,linebuf);
for (x = lineposn; x < 79; x++)
lineout[x-1] = ' ';
lineout[78] = 13;
lineout[79] = 10;
lineout[24] = ' ';
for (x = 0; x <= 79; x++)
if (errorc == 0)
errorc = 41 & biosprint(0,lineout[x],0);

ltoa(status.stockheld, linebuf2, 10);
lineposn = sprintf(lineout,"NUMBER OF SHARES HELD :  %s    ", linebuf2);
for (x = lineposn; x < 59; x++)
lineout[x-1] = ' ';
lineout[59] = 13;
lineout[60] = 10;
lineout[57] = 13;
lineout[58] = 10;
for (x = 0; x <= 60; x++)
if (errorc == 0)
errorc = 41 & biosprint(0,lineout[x],0);

lineposn = 40;
strcpy(lineout, "                      CALL  OPTIONS        ");
lineout[lineposn-1]= 13;
lineout[lineposn] = 10;
for (x = 0; x <= lineposn; x++)
if (errorc == 0)
errorc = 41 & biosprint(0,lineout[x],0);
strcpy(linebuf2," ");
lineposn = sprintf(lineout, "  MONTH %s ", linebuf2);
for (x = lineposn; x < 15; x++)
lineout[x-1] = ' ';
lineout[14] = 0;
lineposn = 14 + sprintf(linebuf," Ex. PRICE%s ", linebuf2);
strcat(lineout,linebuf);
for (x = lineposn; x < 30; x++)
lineout[x-1] = ' ';
lineout[29] = 0;
lineposn = 29 + sprintf(linebuf," VALUE%s ", linebuf2);
strcat(lineout,linebuf);
for (x = lineposn; x < 45; x++)
lineout[x-1] = ' ';
lineout[44] = 0;
lineposn = 44 + sprintf(linebuf," DELTA%s ", linebuf2);
strcat(lineout,linebuf);
for (x = lineposn; x < 60; x++)
lineout[x-1] = ' ';
lineout[59] = 0;
lineposn = 59 + sprintf(linebuf," No. HELD%s ", linebuf2);
strcat(lineout,linebuf);
for (x = lineposn; x < 72; x++)
lineout[x-1] = ' ';
lineout[72] = 13;
lineout[73]= 10;
lineout[74] = 13;
lineout[75] = 10;
lineout[44] = ' ';
lineout[59] = ' ';
lineout[14] = ' ';
lineout[29] = ' ';
for (x = 0; x <= 75; x++)
if (errorc == 0)
errorc = 41 & biosprint(0,lineout[x],0);
for (count = 0; count < 30; count ++)
{
if (status.data[count].month != -1)
{
strncpy(linebuf2, months[status.data[count].month], 3);
lineposn = sprintf(lineout, "  %s ", linebuf2);
for (x = 5 ; x < 15; x++)
lineout[x] = ' ';
lineout[14] = 0;
lineposn = 14 + sprintf(linebuf,"  %-6.2f ", status.data[count].strike);
strcat(lineout,linebuf);
for (x = lineposn; x < 30; x++)
lineout[x-1] = ' ';
lineout[29] = 0;
lineposn = 29 + sprintf(linebuf,"  %-6.2f ", status.data[count].valuec);
strcat(lineout,linebuf);
for (x = lineposn; x < 45; x++)
lineout[x-1] = ' ';
lineout[44] = 0;
lineposn = 44 + sprintf(linebuf,"  %-6.2f ", status.data[count].deltac);
strcat(lineout,linebuf);
for (x = lineposn; x < 60; x++)
lineout[x-1] = ' ';
lineout[59] = 0;
lineposn = 61 + sprintf(linebuf,"    %d        ", status.data[count].heldc);
strcat(lineout,linebuf);
for (x = 70; x < 72; x++)
lineout[x-1] = ' ';
lineout[72] = 13;
lineout[73]= 10;
lineout[14]=' ';
lineout[44] = ' ';
lineout[59] = ' ';
lineout[29] = ' ';
for (x = 0; x <= 73; x++)
if (errorc == 0)
errorc = 41 & biosprint(0,lineout[x],0);
}
}

lineposn = 40;
strcpy(lineout, "                        PUT  OPTIONS        ");
lineout[lineposn-1]= 13;
lineout[lineposn] = 10;
lineout[0] = 13;
lineout[1] = 10;
for (x = 0; x <= lineposn; x++)
if (errorc == 0)
errorc = 41 & biosprint(0,lineout[x],0);
strcpy(linebuf2," ");
lineposn = sprintf(lineout, "  MONTH %s ", linebuf2);
for (x = lineposn; x < 15; x++)
lineout[x-1] = ' ';
lineout[14] = 0;
lineposn = 14 + sprintf(linebuf," Ex. PRICE%s ", linebuf2);
strcat(lineout,linebuf);
for (x = lineposn; x < 30; x++)
lineout[x-1] = ' ';
lineout[29] = 0;
lineposn = 29 + sprintf(linebuf," VALUE%s ", linebuf2);
strcat(lineout,linebuf);
for (x = lineposn; x < 45; x++)
lineout[x-1] = ' ';
lineout[44] = 0;
lineposn = 44 + sprintf(linebuf,"  DELTA%s ", linebuf2);
strcat(lineout,linebuf);
for (x = lineposn; x < 60; x++)
lineout[x-1] = ' ';
lineout[59] = 0;
lineposn = 59 + sprintf(linebuf," No. HELD%s ", linebuf2);
strcat(lineout,linebuf);
for (x = lineposn; x < 72; x++)
lineout[x-1] = ' ';
lineout[72] = 13;
lineout[73]= 10;
lineout[74] = 13;
lineout[75] = 10;
lineout[44] = ' ';
lineout[59] = ' ';
lineout[14] = ' ';
lineout[29] = ' ';
for (x = 0; x <= 75; x++)
if (errorc == 0)
errorc = 41 & biosprint(0,lineout[x],0);
for (count = 0; count < 30; count ++)
{
if (status.data[count].month != -1)
{
strncpy(linebuf2, months[status.data[count].month], 3);
lineposn = sprintf(lineout, "  %s ", linebuf2);
for (x = 5; x < 15; x++)
lineout[x] = ' ';
lineout[14] = 0;
lineposn = 14 + sprintf(linebuf,"  %-6.2f ", status.data[count].strike);
strcat(lineout,linebuf);
for (x = lineposn; x < 30; x++)
lineout[x-1] = ' ';
lineout[29] = 0;
lineposn = 29 + sprintf(linebuf,"  %-6.2f ", status.data[count].valuep);
strcat(lineout,linebuf);
for (x = lineposn; x < 45; x++)
lineout[x-1] = ' ';
lineout[44] = 0;
lineposn = 44 + sprintf(linebuf,"  %-6.2f ", status.data[count].deltap);
strcat(lineout,linebuf);
for (x = lineposn; x < 60; x++)
lineout[x-1] = ' ';
lineout[59] = 0;
lineposn = 61 + sprintf(linebuf,"    %d           ", status.data[count].heldp);
strcat(lineout,linebuf);
for (x = 70; x < 72; x++)
lineout[x-1] = ' ';
lineout[72] = 13;
lineout[73]= 10;
lineout[14]=' ';
lineout[44] = ' ';
lineout[59] = ' ';
lineout[29] = ' ';
for (x = 0; x <= 73; x++)
if (errorc == 0)
errorc = 41 & biosprint(0,lineout[x],0);
}
}
lineout[0] = 13;
lineout[1] = 10;
for (x = 0; x < 2; x++)
if (errorc == 0)
errorc = 41 & biosprint(0,lineout[x],0);

}

int printdetailt()
{
int x;
int errorc;
int finish;
double startprice;
double strikes[30];
double increment;
int numberinc;
char lineout[80];
char linebuf[45];
char linebuf2[40];
char linebuf3[40];
char linebuf4[40];
int mthinc;
int lineposn;
int count;
int	x2;
int x3;
int x4;
int	y2;
int	z2;
char    ch1;
int	done;
int stinc;
double stocksave;
int firstmth;
finish = 0;
makewindow(10,8,60,19,"Shareprice Selection");
stocksave = status.stockprice;
gotoxy(13,11);
cprintf("Start Shareprice     =          ");
startprice = inputreal(wherex(),wherey(), 0.0, 6,2 );
ch1 = getch();
gotoxy(13,13);
cprintf("Price Increment      =          ");
increment = inputreal(wherex(),wherey(), 0.1 , 6,2 );
ch1 = getch();
gotoxy(13,15);
cprintf("Number of Increments =          ");
numberinc = inputinteger(wherex(),wherey(), 1 , 2);
ch1 = getch();


if (biosprint(2,0,0) & 1 == 1)
{
errormessage("Printer Error\nCheck Hardware");
return(0);
}
errorc = 0;
errorc = 41 & biosprint(1,0,0);
errorc = 41 & biosprint(0,13,0);
errorc = 41 & biosprint(0,10,0);
lineposn = sprintf(lineout, "FILE : %s  ", sys.lastfile);
for (x = lineposn; x < 30; x++)
lineout[x-1] = ' ';
lineout[25] = 0;
strcpy(linebuf2,sys.date);
lineposn = 24 + sprintf(linebuf,"DATE : %s  ", linebuf2);
strcat(lineout,linebuf);
for (x = lineposn; x < 59; x++)
lineout[x-1] = ' ';
lineout[50] = 0;
lineposn = 49 + sprintf(linebuf,"INTEREST RATE : %-5.2f \%   ", status.interest);
strcat(lineout,linebuf);
for (x = lineposn; x < 86; x++)
lineout[x-1] = ' ';
lineout[76] = 13;
lineout[77] = 10;
for (x = 0; x <= 77; x++)
if (errorc == 0)
errorc = 41 & biosprint(0,lineout[x],0);
ltoa(totalcalls, linebuf2, 10);
lineposn = sprintf(lineout, "TOT. No. CALLS = %s  ", linebuf2);
for (x = lineposn; x < 29; x++)
lineout[x-1] = ' ';
lineout[25] = 0;
lineposn = 24 + sprintf(linebuf,"TOT. VALUE $ %-10.2f  ", totalvalue);
strcat(lineout,linebuf);
for (x = lineposn; x < 59; x++)
lineout[x-1] = ' ';
lineout[50] = 0;
lineposn = 49 + sprintf(linebuf,"VOLATILITY : %-5.2f \%             ", status.volatility);
strcat(lineout,linebuf);
for (x = 73; x < 76; x++)
lineout[x-1] = ' ';
lineout[76] = 13;
lineout[77] = 10;
lineout[24] = ' ';
for (x = 0; x <= 77; x++)
if (errorc == 0)
errorc = 41 & biosprint(0,lineout[x],0);
ltoa(totalputs, linebuf2, 10);
lineposn = sprintf(lineout, "TOT. No. PUTS  = %s  ", linebuf2);
for (x = lineposn; x < 29; x++)
lineout[x-1] = ' ';
lineout[25] = 0;
lineposn = 24 +  sprintf(linebuf,"TOT. DELTA %-7.2f $/c            ", totaldelta* 0.01);
strcat(lineout,linebuf);
for (x = lineposn; x < 60; x++)
lineout[x-1] = ' ';
lineout[50] = 0;
lineposn = 49 + sprintf(linebuf,"SHARE PRICE  $%-5.2f  ", status.stockprice);
strcat(lineout,linebuf);
for (x = lineposn; x < 79; x++)
lineout[x-1] = ' ';
lineout[78] = 13;
lineout[79] = 10;
lineout[24] = ' ';
for (x = 0; x <= 79; x++)
if (errorc == 0)
errorc = 41 & biosprint(0,lineout[x],0);

ltoa(status.stockheld, linebuf2, 10);
lineposn = sprintf(lineout,"NUMBER OF SHARES HELD :  %s    ",linebuf2);
for (x = lineposn; x < 59; x++)
lineout[x-1] = ' ';
lineout[59] = 13;
lineout[60] = 10;
lineout[57] = 13;
lineout[58] = 10;
for (x = 0; x <= 60; x++)
if (errorc == 0)
errorc = 41 & biosprint(0,lineout[x],0);

done = 0;
firstmth = 1;
x2 = sys.expiry[8].emonth;
while (done != 12)
{
	for (y2 = 0; y2 < 30; y2++)
	{
	  if (x2 == status.data[y2].month )
	  { firstmth = x2;
	    done = 11;
	    }
	 }
	  done++;
	  x2++;
}
gotoxy(1,1);
cprintf(" %d ",firstmth);

x2 = 0;
for (z2 = 0; z2 <30 ; z2++)
  {
  if (status.data[z2].strike > 0.0)
  { done = FALSE;
    for (x3 = 0; x3 < x2; x3++)
    {
    if (strikes[x3] == status.data[z2].strike)
    done = TRUE;
   }
   if (!done)
   {
   strikes[x2] = status.data[z2].strike;
   x2++;
   }

  } /* if */
  }

for (x3 = 0; x3 < x2 ; x3++)
cprintf("\n %6.2f ",strikes[x3]);
/*done = getch();
beep();*/

for (x3 = 0; x3 < numberinc; x3++)
{
lineposn = 40;
lineout[0] = 0;
sprintf(lineout, "\n   If Shareprice is $ %-7.2f                 ",startprice+(increment * x3 ));
status.stockprice = startprice + (increment * x3);
  while (lineout[x] != 0)
  {
if (errorc == 0)
errorc = 41 & biosprint(0,lineout[x],0);
x++;
}
strncpy(linebuf2, months[firstmth], 3);
x4 = firstmth + 3;
if (x4 > 11) x4 = x4 - 12;
strncpy(linebuf3, months[x4], 3);
x4 = firstmth + 6;
if (x4 > 11) x4 = x4 - 12;
strncpy(linebuf4, months[x4], 3);
linebuf2[3] = 0;
linebuf3[3] = 0;
linebuf4[3] = 0;
lineout[0]= 0;
lineposn = sprintf(lineout, "\n                %s                    %s            ", linebuf2,linebuf3);
  while (lineout[x] != 0)
  {
if (errorc == 0)
errorc = 41 & biosprint(0,lineout[x],0);
x++;
}
lineposn = sprintf(lineout, "\n        Call         Put         Call       Put        ");
  while (lineout[x] != 0)
  {
if (errorc == 0)
errorc = 41 & biosprint(0,lineout[x],0);
x++;
}
lineposn = sprintf(lineout, "\nStrike  Value Delta   Value Delta  Value Delta  Value Delta        ");
  while (lineout[x] != 0)
  {
if (errorc == 0)
errorc = 41 & biosprint(0,lineout[x],0);
x++;
}
for ( stinc = 0; stinc < x2; stinc++)
{
    status.data[30].strike = strikes[stinc];

 sprintf(lineout,"\n%-5.2f  ",status.data[30].strike);
 for ( mthinc = 0; mthinc <= 1; mthinc ++)
  {
  status.data[30].month = firstmth + (mthinc* 3);
  if (status.data[30].month > 11) status.data[30].month+= -12;
  calcrow(38);
 sprintf(linebuf2," %-5.2f  %-5.2f  %-5.2f  %-5.2f ",status.data[30].valuec, status.data[30].deltac,
  status.data[30].valuep, status.data[30].deltap);
  strcat(lineout,linebuf2);
  }
  x = 0;
  while (lineout[x] != 0)
  {
if (errorc == 0)
errorc = 41 & biosprint(0,lineout[x],0);
x++;
}






}
}
closewindow(10,8,60,19);

}







void drawheadings()
{
switch (sys.screen)
{
case SCREEN1 :
    disppage();
    textcolor(sys.graphics.colors.heading);
    textbackground(sys.graphics.colors.databack);
    gotoxy(5,5);
    if (sys.display == HELDS)
    cprintf("                                  Value                      Held         ");
    else
    cprintf("                                  Value                      Delta        ");
    gotoxy(5,6);
    cprintf("Month        Strike         Call          Put          Call         Put   ");
    break;
case SCREEN2 :
    textcolor(sys.graphics.colors.heading2);
    textbackground(sys.graphics.colors.databack);
    gotoxy(5,5);
    cprintf("                     Expiry                             Dividend (cents)");
    gotoxy(5,6);
    cprintf("Month      Multiplier       Day        Days-Left       Day        Amount");
    break;
case SCREEN3 :
    disppage();
    textcolor(sys.graphics.colors.heading);
    textbackground(sys.graphics.colors.databack);
    gotoxy(5,5);
    if (sys.display == INVVOL)
    cprintf("                                Volatility               Market Value    ");
    else if (sys.display == OVERVALUED)
    cprintf("                            % Over/UnderValued           Market Value    ");
    gotoxy(5,6);
    cprintf("Month        Strike         Call          Put          Call         Put   ");
    break;
  }
}


void drawcommands()
{
textcolor(sys.graphics.colors.commands);
gotoxy(1,24);
cprintf(
" F1:Help        F2:Save          F3:Recall         F4:Sort           F5:Graph\n");
if (sys.screen != SCREEN3)
cprintf(
" F6:Screen      F7:Clear         F8:Utilities      F9:Delta/Held     F10:Quit");
else
cprintf(
" F6:Screen      F7:Clear         F8:Utilities      F9:InvVol/%Val    F10:Quit");
}


void drawstatus()
{
textcolor(sys.graphics.colors.status);
gotoxy(2,1);
cprintf("OPTIONS ANALYST     %s", startname);
gotoxy(22,2);
cprintf("DATE  =");
gotoxy(22,3);
cprintf("PRICE =");
gotoxy(50,2);
cprintf("INTEREST   :         %");
gotoxy(50,3);
cprintf("VOLATILITY :         %");
gotoxy(8,3);
cprintf("             ");
gotoxy(2,3);
cprintf("File: %s", filename);
}

void genscreen()
{
textcolor(sys.graphics.colors.data);
textbackground(sys.graphics.colors.databack);
window(2,8,79,22);
clrscr();
window(1,1,80,25);
drawborder();
drawheadings();
drawcommands();
drawstatus();
showall();
hidecursor();
}


char screenbuffer[4000];

void makewindow(int x1,int y1,int x2,int y2,char *header)
{
gettext(x1,y1,x2+1,y2+1,screenbuffer);
textbackground(sys.graphics.colors.highback);
window(x1+1,y1+1,x2+1,y2+1);
clrscr();
textcolor(sys.graphics.colors.windowfore);
textbackground(sys.graphics.colors.windowback);
window(x1,y1,x2,y2);
clrscr();
window(x1,y1,x2+1,y2+1);
textlineh(1,1,x2-x1+1);
textlineh(y2-y1+1,1,x2-x1+1);
textlinev(1,1,y2-y1+1,218,192);
textlinev(x2-x1+1,1,y2-y1+1,191,217);
window(1,1,80,25);
gotoxy(x1 + (x2-x1)/2 - strlen(header)/2,y1);
cprintf(" %s ",header);
}

void closewindow(int x1,int y1,int x2,int y2)
{
textcolor(sys.graphics.colors.data);
textbackground(sys.graphics.colors.databack);
puttext(x1,y1,x2+1,y2+1,screenbuffer);
}

void errormessage(char *message)
{
char scbuffer[800];
struct text_info inforec;
int x1 = 10;
int x2 = 10;
int y1 = 20;
int y2 = 23;
if (strchr(message,13) != NULL)
x2 += 2*strlen(message)/3 + 4;
else
x2 += strlen(message)+4;
burp();
gettextinfo(&inforec);
if (x2-x1 < 17) x2 = x1 + 17;
gettext(1,y1,80,y2+1,scbuffer);
textbackground(sys.graphics.colors.highback);
window(x1+1,y1+1,x2+1,y2+1);
clrscr();
textbackground(sys.graphics.colors.highlight);
textcolor(sys.graphics.colors.highback);
window(x1,y1,x2,y2);
clrscr();
window(x1,y1,x2+1,y2+1);
textlineh(1,1,x2-x1+1);
textlineh(y2-y1+1,1,x2-x1+1);
textlinev(1,1,y2-y1+1,218,192);
textlinev(x2-x1+1,1,y2-y1+1,191,217);
window(1,1,80,25);
gotoxy(x1 + (x2-x1)/2 - 5/2,y1);
cprintf(" ERROR ");
window(x1+2,y1+1,x2-2,y2-1);
gotoxy(1,1);
cprintf("%s",message);
window(1,1,80,25);
delay(1500);
puttext(1,y1,80,y2+1,scbuffer);
textattr(inforec.attribute);
gotoxy(inforec.curx,inforec.cury);
window(inforec.winleft,inforec.wintop,inforec.winright,inforec.winbottom);
}


int	choice( int n, char *ch[])
/* first line of array gives the window title */
{
 int	x;
 int	y;
 int	z;
 char	let;
 int    chi;
 int    maxlen;
 int current;
 current = 1;
 maxlen = 0;
 for	( x = 0; x <= n; x++)
	{
	y = strlen(ch[x]);
	if	( maxlen < y )
	maxlen = y;
	}
   if (maxlen < 24  ) maxlen = 24;
   maxlen += 2;
   x = (80 - maxlen)/2;
   y = (12 - n);
   if (y == 7) y = 8;
   makewindow( x-4, y-2, x + maxlen +3, y + (2* n) + 2, ch[0]);
   gotoxy(x-2, y);

   cprintf(" Select a Number or <esc>");
	gotoxy( x-1, y + (2 * (1)) );
	putch(16);
        cprintf("%d ", current);
	cprintf("%s ",ch[current]);
	putch(17);
   for	(z=2 ; z <= n; z++)
       {
	gotoxy( x-1, y + (2 * (z)) );
        cprintf(" %d ", z);
	cprintf("%s",ch[z]);
	}
	do
	{
	let = getch();
	if (let == 0)
	     {
             gotoxy( x-1, y + (2 * (current)) );
	     cprintf(" %d ",current);
	     cprintf("%s   ",ch[current]);
	     chi = getch();
	     switch (chi)
	      {
	       case ARROWD : current++;
			     if (current > n) current = 1;
			     break;
	       case ARROWU : current--;
			     if (current < 1) current = n;
			     break;
		  default  : ungetch(chi);
	       }
			     gotoxy( x-1, y + (2 * (current)) );
			     putch(16);
			     cprintf("%d ",current);
			     cprintf("%s ",ch[current]);
			     putch(17);
               }
	      else
	if ((let > 58  ) && (let <= 58 + n))
	let = let + '0' - 58;
	} while ( (let > '0'+ n || let < '1') && let != 27 && let != 13) ;
        closewindow( x-4, y-2, x+  maxlen + 3, y + (2* n)+2 );
	if (let == 27) return(0);
	else
	if (let == 13) return(current);
	else
	return( let - '0');
}



