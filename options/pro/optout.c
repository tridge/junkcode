#include <fcntl.h>
#include <sys\stat.h>
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
"\n\r"
"\n\r"
"                           The\n\r\n\r"
"                     OPTIONS ANALYST\n\r"
"\n\r\n\r\n\r\n\r"
"                    Copyright (c) 1988\n\r"
"                 EFAM RESOURCES Pty. Ltd.\n\r"
"                   All Rights Reserved\n\r\n\r"
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
	errormessage("Disk access error\n\rDisk probably full");
	else
        strncpy(sys.lastfile,fname,15);
	close(fhandle);
	}
	else
	{
	sprintf(error,"Disk access error\n\r%s",sys_errlist[errno]);
	errormessage(error);
	}
} else errormessage("Drive not ready 11\n\rCheck hardware");
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



/* Prints a copy of the spreadsheet to a file or to the printer
{
 FILE *file;
 int columns, counter1, counter2, counter3, col = 0, row, border, toppage,
  lcol, lrow, dummy, printed, oldlastcol;

 filename[0] = 0;
 writeprompt(MSGPRINT);
 if (!editstring(filename, "", MAXINPUT))
  return;
 if (filename[0] == 0)
  strcpy(filename, "PRN");
 if ((file = fopen(filename, "wt")) == NULL)
 {
  errormsg(MSGNOOPEN);
  return;
 }
 oldlastcol = lastcol;
 for (counter1 = 0; counter1 <= lastrow; counter1++)
 {
  for (counter2 = lastcol; counter2 < MAXCOLS; counter2++)
  {
   if (format[counter2][counter1] >= OVERWRITE)
    lastcol = counter2;
  }
 }
 if (!getyesno(&columns, MSGCOLUMNS))
  return;
 columns = (columns == 'Y') ? 131 : 79;
 if (!getyesno(&border, MSGBORDER))
  return;
 border = (border == 'Y');
 while (col <= lastcol)
 {
  row = 0;
  toppage = TRUE;
  lcol = pagecols(col, border, columns) + col;
  while (row <= lastrow)
  {
   lrow = pagerows(row, toppage, border) + row;
   printed = 0;
   if (toppage)
   {
    for (counter1 = 0; counter1 < TOPMARGIN; counter1++)
    {
     fprintf(file, "\n");
     printed++;
    }
   }
   for (counter1 = row; counter1 < lrow; counter1++)
   {
    if ((border) && (counter1 == row) && (toppage))
    {
     if ((col == 0) && (border))
      sprintf(s, "%*s", LEFTMARGIN, "");
     else
      s[0] = 0;
     for (counter3 = col; counter3 < lcol; counter3++)
     {
      centercolstring(counter3, colstr);
      strcat(s, colstr);
     }
     fprintf(file, "%s\n", s);
     printed++;
    }
    if ((col == 0) && (border))
     sprintf(s, "%-*d", LEFTMARGIN, counter1 + 1);
    else
     s[0] = 0;
    for (counter2 = col; counter2 < lcol; counter2++)
     strcat(s, cellstring(counter2, counter1, &dummy, FORMAT));
    fprintf(file, "%s\n", s);
    printed++;
   }
   row = lrow;
   toppage = FALSE;
   if (printed < 66)
    fprintf(file, "%c", FORMFEED);
  }
  col = lcol;
 }
 fclose(file);
 lastcol = oldlastcol;
}  printsheet */


getoutputname(char filename1[])
{
strcpy(filename1,"PRN");
}


int printdetails()
{
int x;
int errorc;
char filename1[20];
char lineout[80];
char linebuf[45];
char linebuf2[40];
int lineposn;
int count;
FILE *file;
getoutputname(filename1);
/* strcpy(filename1, "PRN");*/
 if (filename1[0] == 0 ||((file = fopen(filename1, "wt")) == NULL) || (ferror(file) != 0))
 {
 errormessage("Printer Error\n\rCheck Hardware");
 return(0);
 }
 makewindow(1,12,77,24,"Please Wait");
gotoxy(6,19);
cprintf("The Data table is now being printed, please wait");
fprintf(file,"\r\nOWNER : %s   \r\n",startname);
strncpy(lineout,sys.date,7);
lineout[7]=0;
if (sys.prt == 1) {fputc(27,file); fputc(5,file); fputc(0,file);}/* IBM */
fprintf(file,"DATE : %s   ", lineout);
if (ferror(file) != 0) return(0);
fprintf(file,"INTEREST RATE : %-5.2f \%    ", status.interest);
fprintf(file,"FILE : %s\r\n", filename);
if (ferror(file) != 0) return(0);
/*ltoa(totalcalls, linebuf2, 10);*/
fprintf(file, "TOT. No. CALLS = %5ld  ", totalcalls);
fprintf(file,"TOT. VALUE $ %-10.2f  ", totalvalue);
fprintf(file,"VOLATILITY : %-5.2f \%     \r\n", status.volatility);
if (ferror(file) != 0) return(0);
/*ltoa(totalputs, linebuf2, 10);*/
fprintf(file, "TOT. No. PUTS  = %5ld  ", totalputs);
fprintf(file,"TOT. DELTA %-7.2f $/c   ", totaldelta* 0.01);
fprintf(file,"SHARE PRICE  $%-5.2f\r\n", status.stockprice);
if (ferror(file) != 0) return(0);
/*ltoa(status.stockheld, linebuf2, 10);*/
fprintf(file,"NUMBER OF SHARES HELD :  %9ld    \r\n", status.stockheld);
fprintf(file,"                     CALL  OPTIONS        PUT  OPTIONS \r\n");
strcpy(linebuf2," ");
fprintf(file," MONTH Ex. PRICE  VALUE  DELTA No. HELD    VALUE DELTA No. HELD\r\n");
if (ferror(file) != 0) return(0);
for (count = 0; count < 30; count ++)
{
if (status.data[count].month != -1)
{
strncpy(linebuf2, months[status.data[count].month], 3);
fprintf(file, "  %s ", linebuf2);
if (ferror(file) != 0) return(0);
fprintf(file,"  %6.2f ", status.data[count].strike);
fprintf(file,"  %6.2f ", status.data[count].valuec);
fprintf(file,"  %4.2f ", status.data[count].deltac);
fprintf(file," %4d  ", status.data[count].heldc);
fprintf(file,"    %6.2f ", status.data[count].valuep);
fprintf(file," %4.2f ", status.data[count].deltap);
fprintf(file," %4d \r\n", status.data[count].heldp);
if (ferror(file) != 0) return(0);
}
}
fclose(file);
closewindow(1,12,77,24);
}
double startprice;
double incprice;

getprices()
{
char ch;
makewindow(8,12,50,19,"Price Choice");
gotoxy(10,15);
cprintf("Enter the start price     : $       ");
retcursor();
startprice = inputreal(45,15,0.0,6,2);
ch =getch();
if (ch == 0) ch = getch();
gotoxy(10,18);
cprintf("Enter the price increment : $       ");
incprice = inputreal(45,18,0.0,6,2);
hidecursor();
closewindow(8,12,50,19);
}


int printdetailt()
{
int x;
int errorc;
char ch1;
char filename1[20];
char lineout[80];
char linebuf[45];
char linebuf2[40];

double p1,p2,p3,p4,p5;
double statusprice;
int lineposn;
int count;
FILE *file;
startprice = 1.00;
incprice = 0.5;
getprices();

statusprice = status.stockprice;
getoutputname(filename1);
/* strcpy(filename1, "PRN");*/
 if (filename1[0] == 0 ||((file = fopen(filename1, "wt")) == NULL) || (ferror(file) != 0))
 {
 errormessage("Printer Error\n\rCheck Hardware");
 return(0);
 }
 makewindow(1,12,77,24,"Please Wait");
gotoxy(6,19);
cprintf("The Data table is now being printed, please wait");
if (sys.prt == 1) {fputc(27,file); fputc(5,file); fputc(0,file);}
fprintf(file,"\r\nOWNER : %s   \r\n",startname);
strncpy(lineout,sys.date,7);
lineout[7]=0;
fprintf(file,"DATE : %s   ", lineout);
if (ferror(file) != 0) return(0);
fprintf(file,"FILE : %s   ", filename);
fprintf(file,"VOLATILITY : %-6.2f \%  \r\n", status.volatility);
if (ferror(file) != 0) return(0);
fprintf(file,"                 VALUE CALL OPTIONS             VALUE PUT OPTIONS \r\n");
fprintf(file," SHAREPRICE%5.2f %5.2f %5.2f %5.2f %5.2f  %5.2f %5.2f %5.2f %5.2f %5.2f\r\n",
startprice,startprice+incprice,startprice+2*incprice,startprice+3*incprice,startprice+4*incprice,
startprice,startprice+incprice,startprice+2*incprice,startprice+3*incprice,startprice+4*incprice);
for (count = 0; count < 30; count ++)
{
if (status.data[count].month != -1)
{
strncpy(linebuf2, months[status.data[count].month], 3);
linebuf2[3]=0;
fprintf(file, " %s", linebuf2);
fprintf(file,"%6.2f", status.data[count].strike);
status.data[30].month = status.data[count].month;
status.data[30].strike = status.data[count].strike;
status.stockprice = startprice;
calcrow(38);
fprintf(file," %5.2f", status.data[30].valuec);
p1= status.data[30].valuep;
status.stockprice = startprice+incprice;
calcrow(38);
fprintf(file," %5.2f", status.data[30].valuec);
if (ferror(file) != 0) return(0);
p2= status.data[30].valuep;
status.stockprice = startprice+2*incprice;
calcrow(38);
fprintf(file," %5.2f", status.data[30].valuec);
p3= status.data[30].valuep;
if (ferror(file) != 0) return(0);
status.stockprice = startprice+3*incprice;
calcrow(38);
fprintf(file," %5.2f", status.data[30].valuec);
p4= status.data[30].valuep;
status.stockprice = startprice+4*incprice;
calcrow(38);
fprintf(file," %5.2f", status.data[30].valuec);
if (ferror(file) != 0) return(0);
p5= status.data[30].valuep;
fprintf(file,"  %5.2f",p1);
fprintf(file," %5.2f",p2);
fprintf(file," %5.2f",p3);
fprintf(file," %5.2f",p4);
fprintf(file," %5.2f\r\n",p5);
if (ferror(file) != 0) return(0);
}
}
ch1 = 12;
fputc(ch1,file);
fclose(file);
closewindow(1,12,77,24);
status.stockprice = statusprice;
}
int printdetailu()
{
int x;
int errorc;
char ch1;
char filename1[20];
char lineout[80];
char linebuf[45];
char linebuf2[40];

double statusprice;
int lineposn;
int count;
FILE *file;
int incmul;
startprice = 1.00;
incprice = 0.5;

getprices();

statusprice = status.stockprice;
getoutputname(filename1);
/* strcpy(filename1, "PRN");*/
 if (filename1[0] == 0 ||((file = fopen(filename1, "wt")) == NULL) || (ferror(file) != 0))
 {
 errormessage("Printer Error\n\rCheck Hardware");
 return(0);
 }
 makewindow(1,12,77,24,"Please Wait");
gotoxy(6,19);
cprintf("The Data table is now being printed, please wait");
fprintf(file,"\r\nOWNER : %s   \r\n",startname);
strncpy(lineout,sys.date,7);
lineout[7]=0;
fprintf(file,"DATE : %s   ", lineout);
if (ferror(file) != 0) return(0);
fprintf(file,"FILE : %s  ", filename);
fprintf(file,"VOLATILITY : %-5.2f \%  \r\n", status.volatility);
if (ferror(file) != 0) return(0);
fprintf(file,"                             VALUE CALL OPTIONS  \r\n");
if (ferror(file) != 0) return(0);
fprintf(file," SHAREPRICE%5.2f %5.2f %5.2f %5.2f %5.2f %5.2f %5.2f %5.2f %5.2f %5.2f\r\n",
startprice,startprice+incprice,startprice+2*incprice,startprice+3*incprice,startprice+4*incprice,
startprice+5*incprice,startprice+6*incprice,startprice+7*incprice,startprice+8*incprice,startprice+9*incprice);
for (count = 0; count < 30; count ++)
{
if (status.data[count].month != -1)
{
strncpy(linebuf2, months[status.data[count].month], 3);
linebuf2[3]=0;
fprintf(file, " %s", linebuf2);
fprintf(file,"%6.2f", status.data[count].strike);
status.data[30].month = status.data[count].month;
status.data[30].strike = status.data[count].strike;
for (incmul = 0; incmul <= 9; incmul++)
{
status.stockprice = startprice +incprice * incmul;
calcrow(38);
fprintf(file," %5.2f", status.data[30].valuec);
}
if (ferror(file) != 0) return(0);
fprintf(file,"\r\n");
}
}
fprintf(file,"                             VALUE PUT OPTIONS \r\n");
fprintf(file," SHAREPRICE%5.2f %5.2f %5.2f %5.2f %5.2f %5.2f %5.2f %5.2f %5.2f %5.2f\r\n",
startprice,startprice+incprice,startprice+2*incprice,startprice+3*incprice,startprice+4*incprice,
startprice+5*incprice,startprice+6*incprice,startprice+7*incprice,startprice+8*incprice,startprice+9*incprice);
for (count = 0; count < 30; count ++)
{
if (status.data[count].month != -1)
{
strncpy(linebuf2, months[status.data[count].month], 3);
linebuf2[3]=0;
fprintf(file, " %s", linebuf2);
if (ferror(file) != 0) return(0);
fprintf(file,"%6.2f", status.data[count].strike);
if (ferror(file) != 0) return(0);
status.data[30].month = status.data[count].month;
status.data[30].strike = status.data[count].strike;
if (ferror(file) != 0) return(0);
for (incmul = 0; incmul <= 9; incmul++)
{
status.stockprice = startprice +incprice * incmul;
calcrow(38);
fprintf(file," %5.2f", status.data[30].valuep);
}
if (ferror(file) != 0) return(0);
fprintf(file," \r\n");
}
}
ch1 = 12;
fputc(ch1,file);
fclose(file);
closewindow(1,12,77,24);
status.stockprice = statusprice;
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
" F1:Help        F2:Save          F3:Recall         F4:Sort           F5:Graph\n\r");
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


int choice(int n,char *ch[])
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