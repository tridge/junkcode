/* options analyst program -- main source   */
/* started 14/5/88.        A and P Tridgell */
/* First version completed and manual printed 18/7/88 */

#define MAIN


#include <conio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <graphics.h>
#include <stdio.h>

#include "optdef.h"
#include "options.h"



int tosavesys = FALSE;
double totalvalue,totaldelta,weightedvol;

char months[12][3]
= {"JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC"};



char filename[20] = "DEMO";

int numpts  = 30;
pagetype page = PAGEUP;

int syear;
int smonth;
int sday;

long totalcalls;
long totalputs;
double weightedvolc,weightedvolp;
int recalcvolvalues = TRUE;

static char *utilchoice[8] =
{{"Utilities"},
 {"Directory"},
 {"Bonus issue"},
 {"Cash issue"},
{"Graph resolution"},
{"Print details"},
{"Delete File"},
{"Set Up"}
};

static char *printchoice[4] =
{{"Print Style Choice"},
{"Position Style"},
{"Short Data"},
{"Long Data"}
};

static char *clearchoice[4] =
{{"Clearing Utility"},
  {"Clear Row"},
  {"Clear Column"},
  {"Clear All"}
  };

static char *resolutionchoice[4] =
{{"Graph resolution"},
 {"Coarse graph"},
 {"Medium graph"},
 {"Fine graph"}
 };

static char *quitchoice[4] =
{{"Quit?"},
 {"I wish to order"},
 {"Quit"},
 {"Return to Program"}
 };

static char *graphchoice[7] =
{{"Graphing"},
 {"Whole Position - Price"},
 {"               - Time"},
 {"This Option    - Price"},
 {"               - Time"},
 {"Previous       - Price"},
 {"               - Time"}};


static char *screenchoice[4] =
{{"Screens"},
 {"Main Screen"},
 {"Dividends"},
 {"Inversion"}
 };

static char *payment1[5] =
{{"I wish to Pay by :"},
{"Cheque"},
{"Bankcard"},
{"Mastercard"},
{"Visa card"}
};


/* COLOR CGA CARD */
graphicstype ccgagraphics =
	{"CCGA",
	 {RED,YELLOW,LIGHTGREEN,LIGHTGRAY,LIGHTCYAN,LIGHTBLUE,
			  BLACK,RED,LIGHTGRAY,LIGHTGRAY,BLUE},
         {-1,0x0608},
	 {LIGHTGRAY,1,3,2,2,2,2,2},
	 {1,2,1,1,4},
	 {1,4,3}};

/* MONO CGA CARD */
graphicstype mcgagraphics =
        {"MCGA",
	{ LIGHTGRAY,LIGHTGRAY,LIGHTGRAY,LIGHTGRAY,LIGHTGRAY,
	LIGHTGRAY,BLACK,BLACK,
	LIGHTGRAY,LIGHTGRAY,BLACK},
         {-1,1750},
	 {LIGHTGRAY,1,1,1,1,1,1,1},
	 {1,2,1,3,4},
	 {1,4,3}};

/* HERCULES CARD */
graphicstype hercgraphics =
        {"HERC",
	{ LIGHTGRAY,LIGHTGRAY,LIGHTGRAY,LIGHTGRAY,LIGHTGRAY,
	LIGHTGRAY,BLACK,BLACK,
	LIGHTGRAY,LIGHTGRAY,BLACK},
         {-1,1750},
	 {0,1,1,1,1,1,1,1},
	 {1,2,1,3,4},
	 {2,4,4}};

/* COLOR EGA CARD */
graphicstype cegagraphics =
	{"CEGA",
	{RED,YELLOW,LIGHTGREEN,LIGHTGRAY,LIGHTCYAN,LIGHTBLUE,
			  BLACK,RED,LIGHTGRAY,LIGHTGRAY,BLUE},
         {2304,1750},
	 {BLACK,GREEN,YELLOW,LIGHTRED,BLUE,CYAN,LIGHTMAGENTA,LIGHTGREEN},
	 {1,2,1,1,4},
	 {2,4,4}};

/* MONO EGA CARD */
graphicstype megagraphics =
     {"MEGA",
     { LIGHTGRAY,LIGHTGRAY,LIGHTGRAY,LIGHTGRAY,LIGHTGRAY,
	LIGHTGRAY,BLACK,BLACK,
	LIGHTGRAY,LIGHTGRAY,BLACK},
         {2304,1750},
	 {BLACK,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE},
	 {1,2,1,3,4},
	 {2,4,4}};



statustype status
=	{
        "OPTIONS ANALYST V1.0",
        "EFAM RESOURCES"};

systype sys
=       {
        "OPTIONS SYSTEM V1.0",
        "01JAN80",
	 1,
	 "SAMPLE.SAV",
	 {8,MONTH},
	 SCREEN1,
	 2,
	 HELDS,
       };


divtype divmatrix[27];

char codename[65] =  "AAAAA";
char startname[65] =  "\x20\x8\DEMONSTRATION VERSION";


/* FUNCTIONS */


void demo()
{
static char *demotext =
"   This is a demonstration version of the Options Analyst.\n\r"
"In this version you may not save or recall positions to\n\r"
"disk or alter the stockprice. In the full version, of \n\r"
"course, there is no such restriction.\n\r"
"\n\r"
"   Please make un-altered copies of this program and\n\r"
"distribute them. This version, however, may be used for \n\r"
"NON-COMMERCIAL purposes only and remains the property \n\r"
"of EFAM RESOURCES\n\r"
"\n\r"
"   TO BUY YOUR COPY CONTACT : IS2 Pty. Ltd.\n\r"
"PO. BOX  H37 Australia Square Sydney, N.S.W. 2000.\n\r"
"Telephone (02) 27 4686  Fax: 27 5898\n\r"
"                                           PRESS ANY KEY";

showhelp("DEMONSTRATION VERSION",demotext);
}

void demo1()
{
static char *demo1text =
"   This Function allows tables of values to printed. An   \n\r"
"initial stockprice is entered, and a stockprice increment.\n\r"
"These tables show the value of the options for either 5 or\n\r"
"10 shareprices. This function is useful if you are going\n\r"
"somewhere where you do not have access to a computer.\n\r"
"\n\r"
"   Please make un-altered copies of this program and\n\r"
"distribute them. This version, however, may be used for \n\r"
"NON-COMMERCIAL purposes only and remains the property \n\r"
"of EFAM RESOURCES\n\r"
"   TO BUY YOUR COPY CONTACT : IS2 Pty. Ltd.\n\r"
"PO. BOX  H37 Australia Square Sydney, N.S.W. 2000.\n\r"
"Telephone (02) 27 4686  Fax: 27 5898\n\r"
"                                           PRESS ANY KEY";

showhelp("DEMONSTRATION VERSION",demo1text);
}



int inputstring1(int x,int y,char far *string)
{
char ch;
char instring[50];
strcpy(instring,string);
do
{
gotoxy(x,y);
cprintf("%s",instring);
ch = getch();
if (isalpha(ch)) ch= toupper(ch);
if (( (ch  >= 32) && (ch < 100))
   && strlen(instring) < 49)
	{
	 instring[strlen(instring)+1] = 0;
	 instring[strlen(instring)] = ch;
	}
else
if (ch == 8 && strlen(instring) > 0)
{
     instring[strlen(instring)-1] = 0;
     gotoxy(wherex()-1,y);
     cprintf(" ");
     gotoxy(wherex()-1,y);
}
} while (ch != 13 && ch != 27 && ch != 0);
if (ch == 27)
{
ungetch(ch);
return(-1);
}
else
{
strcpy(string,instring);
ungetch(ch);
return(1);
}
}
static char name[50];
static char address[50];
static char city[50];
static char postcode[50];
static char payment[50];
static char crnumber[50];
static char crdate[50];
static char phonenumber[50];

entercrline( int cline)
{
char *cch;
switch (cline)
  {
       case 13 : inputstring1(24,13,name); break;
       case 14 : inputstring1(24,14,address); break;
       case 15 : inputstring1(24,15,city); break;
       case 16 : inputstring1(24,16,postcode); break;
       case 17 : inputstring1(24,17,phonenumber); break;
       case 19 : inputstring1(24,19,crnumber); break;
       case 20 : inputstring1(24,20,crdate); break;
  }
}

order()
{
int currentrow;
int cardused;
char ch;
char filename1[10];
int done;
int old;
FILE *file;
cardused = FALSE;
done = FALSE;
window(1,1,80,25);
textcolor(sys.graphics.colors.data);
textbackground(sys.graphics.colors.databack);
clrscr();
gotoxy(20,3);
textcolor(sys.graphics.colors.border);
cprintf("The Option Analyst - Order Form");
gotoxy(10,23); cprintf("This introductory offer only valid till December 31, 1988");
textcolor(sys.graphics.colors.data);
textlineh(4,20,50);
gotoxy(10,25); cprintf("  Press F7 to print order form  or  <esc> to exit");
gotoxy(10,6);
textcolor(sys.graphics.colors.highback);
cprintf("Yes, I would like to order my copy of \"The Option Analyst\"");
gotoxy(10,7);
cprintf("At the Special Introductory price of $495 including handling and\n\r");
gotoxy(10,8);
cprintf("Courier delivery within Australia." );
gotoxy(10,10);
cprintf("To create and print the order form, fill in the details below");
gotoxy(10,11);
cprintf("then press F7 to print the form, or <Esc> to exit");
showcursor();
gotoxy(10,13);
cprintf("Name        : ");
gotoxy(10,14);
cprintf("Address     : ");
gotoxy(10,15);
cprintf("Suburb      : ");
gotoxy(10,16);
cprintf("Postcode    : ");
gotoxy(10,17);
cprintf("Phone No.   : ");
gotoxy(10,18);
cprintf("Payment by  : ");
currentrow = 13;
entercrline(currentrow);
do
{ ch = getch();
  if (ch == 0) ch = getch();
  switch (ch)
   {  case ARROWD :
      case 13 : switch (currentrow)
		{
		  case 13 :
		  case 19 :
		  case 14 :
		  case 16 :
		  case 15 : currentrow++; entercrline(currentrow); break;
		  case 17 : hidecursor(); cardused = FALSE;
			   switch (choice(4,payment1))
			   {
			    case 1 : strcpy(payment,"Cheque made out to IS2 Pty. Ltd. "); break;
			    case 2 : cardused = TRUE;
				     strcpy(payment,"Bankcard                         "); break;
		            case 3 : cardused = TRUE;
				     strcpy(payment,"Mastercard                       ");break;
			    case 4 : cardused = TRUE;
				     strcpy(payment,"Visa card                        "); break;
			    case -1: break;
			   }  showcursor();
			   textcolor(sys.graphics.colors.highback);
			    textbackground(sys.graphics.colors.databack);
			      gotoxy(24,18);
			       cprintf(payment);
			       if (cardused) { gotoxy(10,19);
			       cprintf("Card Number : ");cprintf(crnumber);
			       gotoxy(10,20);
			       cprintf("Expiry Date : "); cprintf(crdate);
			       currentrow = 19;}
			       else
			       {currentrow = 13;
			       gotoxy(2,19);
			       clreol();
			       gotoxy(2,20);
			       clreol();
			       cardused = FALSE;
			       }
			    entercrline(currentrow);
			    break;
		   case 20 :  currentrow = 13; entercrline(currentrow); break;
		  } break;
       case ARROWU : switch (currentrow)
		{ case 20 :
		  case 16 :
		  case 17 :
		  case 14 :
		  case 15 : currentrow--; entercrline(currentrow); break;
		  case 19 : old = 19;
		  case 13 : if (cardused  && (old != 19)) {currentrow = 20;
			    entercrline(currentrow);}
			    else
			    { hidecursor(); cardused = FALSE;
			    switch (choice(4,payment1))
			   {
			    case 1 : strcpy(payment,"Cheque made out to IS2 Pty. Ltd"); break;
			    case 2 : strcpy(payment,"Bankcard                       ");
				     cardused= TRUE; break;
		            case 3 : strcpy(payment,"Mastercard                     ");
				     cardused= TRUE; break;
			    case 4 : strcpy(payment,"Visa card                      ");
				     cardused= TRUE; break;
			   }
			    textcolor(sys.graphics.colors.highback);
			    textbackground(sys.graphics.colors.databack);
			      gotoxy(24,18);
			       cprintf(payment);
			     showcursor();
			       if (cardused) { gotoxy(10,19);
			       cprintf("Card Number : ");cprintf(crnumber);
			       gotoxy(10,20);
			       cprintf("Expiry Date : "); cprintf(crdate);
			       if (old == 19) currentrow = 17; else
			       currentrow = 19; old = 0;}
			       else
			       {currentrow = 17;
			       gotoxy(2,19);
			       clreol();
			       gotoxy(2,20);
			       clreol();
			       old = 0;
			       cardused = FALSE;
			       }
			    entercrline(currentrow);
			   } break;
		  } break;
       case 27  : return(0);
       case F07 : strcpy(filename1, "PRN");
		 if (filename1[0] == 0 ||((file = fopen(filename1, "wt")) == NULL) || (ferror(file) != 0))
		 {
		 errormessage("Printer Error\n\rCheck Hardware");
		 return(0);
		 }
		 fprintf(file,"\r\n   To :  IS2 Pty. Ltd.\r\n");
		 fprintf(file,    "         PO. BOX  H37 Australia Square\r\n");
		 fprintf(file,    "         Sydney, N.S.W. 2000.\n\r\n\r\n");
		 fprintf(file,"   I wish to purchase a copy of \"The Option Analyst\" at the  Special\n");
		 fprintf(file," introductory price of $495, including courier delivery.\n\r\n");
                 fprintf(file,"      Name     : %s \n\r\n",name);
		 fprintf(file,"      Address  : %s \n\r\n",address);
		 fprintf(file,"                 %s, %s\n\r\n", city,postcode);
		 fprintf(file,"      Phone No.: %s \n\r\n",phonenumber);
                 fprintf(file,"   Payment by  : %s\n\r\n",payment);
		 if (payment[0] != 'C')
		 {
                 fprintf(file,"   Card Number : %s\r\n\r\n",crnumber);
		 fprintf(file,"   Card Expiry Date : %s\r\n\r\n", crdate);
		 }
		 fprintf(file,"\r\n\r       Signed  : ________________________________\n\r\n");
		done = TRUE; break;
   }
}
while (!done);
textbackground(sys.graphics.colors.databack);
clrscr();
}



void clearrow()
{
celltype 	cell;
	textbackground(sys.graphics.colors.databack);
	status.data[sys.cell.row-8].month = -1;
	status.data[sys.cell.row-8].strike= 0;
	status.data[sys.cell.row-8].heldc= 0;
	status.data[sys.cell.row-8].heldp = 0;
	status.data[sys.cell.row-8].valuec=0;
	status.data[sys.cell.row-8].valuep=0;
        status.data[sys.cell.row-8].volc= 0;
	status.data[sys.cell.row-8].volp = 0;
	status.data[sys.cell.row-8].marketc=0;
	status.data[sys.cell.row-8].marketp=0;
	cell.row= sys.cell.row;
	for (cell.col = MONTH ; cell.col <= HELDP ;cell.col++)
	blankcell(cell);
	for (cell.col = VOLC ; cell.col <= MARKETP ;cell.col++)
	blankcell(cell);
	 }



char editcell(celltype cell)
{
int x,y;
long z;
char ch;
char sharestring[12];
int  pageint;
if (page == PAGEUP || sys.screen == SCREEN2 || cell.row < 8) pageint = 0;
else pageint = 15;
x = endcell(cell.col)-1;
y = cell.row - pageint ;

   switch (cell.col)
       {case MONTH :  entercell(sys.cell);
		     if (status.data[cell.row-8].month == -1 &&
			 cell.row != 8 )
			 status.data[cell.row-8].month =
			inputmonth(x-2,y,(status.data[cell.row-9].month)-1);
			else
			 status.data[cell.row-8].month =
			inputmonth(x-2,y,status.data[cell.row-8].month);
		     wait();
		     if (sys.screen == SCREEN3) calcvolrow(cell.row);
                     calcrow(cell.row);
		     showrow(cell.row);
		     if ( cell.row != 22 && cell.row != 37)
		     {
		     cell.row += 1;
		     showcell(cell);
		     cell.row = cell.row -1;
		     }
		      /* show date of line below if it
			    had been hidden by showall suppression */
		     break;
	case STRIKE : entercell(sys.cell);
		      if (status.stockprice != 4.0) exit(0);
		      status.data[cell.row-8].strike =
		      inputreal(x,y,status.data[cell.row-8].strike,8,sys.decimal);
		      wait();
		     if (sys.screen == SCREEN3) calcvolrow(cell.row);
		      calcrow(cell.row);
                      showrow(cell.row);
		      break;
	case VALUEC : getch();break;
	case VALUEP : getch();break;
	case HELDC : entercell(sys.cell);
		     if (status.stockprice != 4.0) exit(0);
		     status.data[cell.row-8].heldc =
		     inputinteger(x,y,status.data[cell.row-8].heldc,4);
		     wait();
		     showcell(sys.cell);
		     break;
	case HELDP : entercell(sys.cell);
		     if (status.stockprice != 4.0) exit(0);
		     status.data[cell.row-8].heldp =
		     inputinteger(x,y,status.data[cell.row-8].heldp,4);
		     wait();
		     showcell(sys.cell);
		     break;
	case STOCKHELD :
			entercell(sys.cell);
			z = status.stockheld;
		     z =
		     inputlint(x,y,&z,8);
		     wait();
		     break;
	case SHAREPRICE : entercell(sys.cell);
			status.stockprice = 4.0;
			getch();
			demo();
			getch();
			break;
        case VOLATILITY : entercell(sys.cell);
			status.volatility =
			inputreal(x,y,status.volatility,5,2);
			wait();
			calcall();
			calcalloverval();
			showall();
			break;
        case INTEREST : entercell(sys.cell);
			status.interest=
			inputreal(x,y,status.interest,5,2);
			wait();
			initdivmtx(0);
			calcall();
			if (sys.screen == SCREEN3) calcallvol();
			else recalcvolvalues = TRUE;
			showall();
			break;
        case DATE    : 	entercell(sys.cell);
			inputdate(x,y,sys.date);
			wait();
			initdates();
			initsizepay();
			calcdays();
			initdivmtx(0);
			calcall();
			if (sys.screen == SCREEN3) calcallvol();
			else recalcvolvalues = TRUE;
			showall();
			break;
	case SHARESPER : entercell(sys.cell);
			status.sizepay[y].sharesper=
			inputinteger(x,y,status.sizepay[y].sharesper,4);
                        if (status.sizepay[y].sharesper < 1)
			status.sizepay[y].sharesper = 1;
			wait();
                        break;
	case EXPIRYDAY : entercell(sys.cell);
			sys.expiry[y].eday =inputinteger(x,y,sys.expiry[y].eday,2);
			if (sys.expiry[y].eday < 1) sys.expiry[y].eday = 1;
			else if (sys.expiry[y].eday > 31) sys.expiry[y].eday = 28;
			wait();
			initdates();
			calcdays();
			initdivmtx(0);
			calcall();
			tosavesys = TRUE;
		      	showall();
			break;
	case DIVIDENDDAY : entercell(sys.cell);
			status.sizepay[y].dday=inputinteger(x,y,status.sizepay[y].dday,2);
			if (status.sizepay[y].dday < 0)
			status.sizepay[y].dday = 0;
			else if (status.sizepay[y].dday > 31)
			status.sizepay[y].dday = 28;
			wait();
			calcdays();
			initdivmtx(0);
                        calcall();
			showrow(cell.row);
                        break;
	case DIVIDENDCENTS : entercell(sys.cell);
			status.sizepay[y].payout
			= inputinteger(x,y,status.sizepay[y].payout,3);
			if (status.sizepay[y].payout < 0)
			status.sizepay[y].payout = 0;
			wait();
			initdivmtx(0);
                        calcall();
			showrow(cell.row);
			break;
	case VOLC :     getch();
			break;
	case VOLP :     getch();
			break;
	case MARKETC : entercell(sys.cell);
			if (status.stockprice != 4.0) exit(0);
		      status.data[cell.row-8].marketc =
		      inputreal(x,y,status.data[cell.row-8].marketc,8,sys.decimal);
		      if (status.data[cell.row-8].marketc < 0)
		      status.data[cell.row-8].marketc = 0;
			wait();
			status.data[cell.row-8].volc = invertvolc(cell.row);
			calcoverval(cell.row);
			showrow(cell.row);
			break;
	case MARKETP : entercell(sys.cell);
		      if (status.stockprice != 4.0) exit(0);
		      status.data[cell.row-8].marketp =
		      inputreal(x,y,status.data[cell.row-8].marketp,8,sys.decimal);
		      if (status.data[cell.row-8].marketp < 0)
		      status.data[cell.row-8].marketp = 0;
			wait();
			status.data[cell.row-8].volp = invertvolp(cell.row);
			calcoverval(cell.row);
			showrow(cell.row);
			break;
       }
       entercell(cell);
       totals();
       showtotals();
       if (status.stockprice != 4.0) exit(0);
       ready();
       ch = getch();
       return(ch);
}




void docommand(char ch)
{
int count;
char dateb[7];
switch (ch)
       	{
	 case ARROWR : if (status.stockprice != 3 + 1) exit(0);goright(); break;
         case ARROWL : if (status.stockprice != 5 - 1) exit(0);goleft(); break;
	 case ARROWU : if (status.stockprice != 2 + 2) exit(0);goup(); break;
	 case ARROWD : if (status.stockprice != 3 + 1) exit(0);godown(); break;
	 case HOME :  leavecell(sys.cell);
		     if ((sys.screen == SCREEN1 || sys.screen == SCREEN3)
			  && page == PAGEDOWN)
		     {
		     sys.cell.row = 23;
		     sys.cell.col = MONTH;
		     }
		     else
                     if (sys.screen == SCREEN2)
		     {
		     sys.cell.row = 8;
		     sys.cell.col = SHARESPER;
		     }
		     else
		     {
		     sys.cell.row = 8;
		     sys.cell.col = MONTH;
		     }
		     entercell(sys.cell);
		     break;
	 case END :  leavecell(sys.cell);
		     if ((sys.screen == SCREEN1 || sys.screen == SCREEN3)
			  && page == PAGEDOWN)
		     {
		     sys.cell.row = 37;
		     sys.cell.col = MONTH;
		     }
		     else
                     if (sys.screen == SCREEN2)
		     {
		     sys.cell.row = 22;
		     sys.cell.col = SHARESPER;
		     }
		     else
		     {
		     sys.cell.row = 22;
		     sys.cell.col = MONTH;
		     }
		     entercell(sys.cell);
		     break;

	 case PGUP	: if ((sys.screen == SCREEN1 || sys.screen == SCREEN3)
			      && page == PAGEDOWN)
			{
			page = PAGEUP;
			if (sys.cell.row >= 8)
			sys.cell.row -= 15;
			disppage();
			}
			showall();
			entercell(sys.cell);
			break;
	 case PGDN	: if ((sys.screen == SCREEN1 || sys.screen == SCREEN3)
				&& page == PAGEUP)
			{
			page = PAGEDOWN;
			if (sys.cell.row >= 8)
			sys.cell.row += 15;
                        disppage();
			}
			showall();
			entercell(sys.cell);
			break;
         case F01 :	help();break;
         case F02 :     demo();
			break;
	 case F03 :     demo();
			break;
         case F04 : sortstatus(); genscreen(); entercell(sys.cell); break;
         case F05 :
		{
		    if (drawgraph(choice(6,graphchoice)) < 0)
		    errormessage("Graphics initialisation error");
		    genscreen();
		    showtotals();
		    entercell(sys.cell);
		 }
		    break;
         case F06 :
         	    leavecell(sys.cell);
		 switch (choice(3,screenchoice))
		 {
 		 case 1 :
		      if (tosavesys) savesys();
		      sys.screen = SCREEN1;
		      page = PAGEUP;
		      sys.cell.col = MONTH;
      		      sys.cell.row = 8;
		      sys.display = HELDS;
		      genscreen();
		      break;
		 case 2 :
		      sys.screen = SCREEN2;
		      sys.display = HELDS;
		      page = PAGEUP;
      		      sys.cell.row = 8;
		      sys.cell.col = EXPIRYDAY;
		      genscreen();
		      break;
		 case 3 :
		      if (tosavesys) savesys();
		      sys.cell.col = MONTH;
		      page = PAGEUP;
		      sys.screen = SCREEN3;
		      sys.display = INVVOL;
      		      sys.cell.row = 8;
		      wait();
		      if (recalcvolvalues)
		      calcallvol();
		      ready();
		      genscreen();
		      break;
		 }
		 showtotals();
		 entercell(sys.cell);
		 break;
            case F07 :
		   switch(choice(3,clearchoice))
		   {
		  case 1 :  if (sys.cell.row < 8 || sys.screen == SCREEN2) break;
			    wait();
			    clearrow();
				 calcall();
				 showall();
				 totals();
                              entercell(sys.cell);
			      break;
		   case 2 : wait();
			    switch( sys.cell.col)
			    {
                            case VOLC :
                            	case VALUEC :
				case VOLP :
				case DELTAC :
				case VALUEP :
                            	case DELTAP:
                            	case SHAREPRICE :break;
				case STOCKHELD : status.stockheld = 0; break;
				case VOLATILITY : status.volatility = 0.0;break;
				case INTEREST   : status.interest = 0.0; break;
				case  DATE      :
				case YEARMONTH :
				case SHARESPER : for (count =0;count <24; count++)
						   status.sizepay[count].sharesper = 1000;
						   break;               
				case EXPIRYDAY :
				case DAYSLEFT :
                            case MONTH : break;
                            case STRIKE : for (count = 0;count < 30; count++)
                                          {
					  status.data[count].strike = 0.0;
					  status.data[count].volc = 0.0;
					  status.data[count].volp = 0.0;
					  }
					  break;
				case MARKETC : for (count = 0;count < 30; count++)
                                        {
					status.data[count].marketc = 0.0;
					status.data[count].volc = 0.0;
					}
					  break;
				case HELDC :  for (count = 0;count < 30; count++)
					  status.data[count].heldc = 0;
					  break;
				case MARKETP : for (count = 0;count < 30; count++)
                                          {
					  status.data[count].marketp = 0.0;
					  status.data[count].volp = 0.0;
					  }
					  break;
				case HELDP :  for (count = 0;count < 30; count++)
					  status.data[count].heldp = 0;
					  break;
				case DIVIDENDDAY : for (count = 0;count < 24; count++)
					  status.sizepay[count].dday = 0;
					  break;
			        case DIVIDENDCENTS :  for (count = 0;count < 24; count++)
					  status.sizepay[count].payout = 0;
					  break;

			    }  wait();
				 calcall();
				 showall();
				 totals();  break;
			  case 3:   clearall();wait();   wait();
				 calcall();
				 showall();
				 totals();break;
			  /* clear all set shares per con */


		    }           showtotals();
				 ready();
				 entercell(sys.cell);
				 /* inverse vol now invalid */
		    break;


         case F08 :
			leavecell(sys.cell);
		   switch( choice(7, utilchoice))
		      {
			case 1 : getdir(); break;
			case 2 : bonusissue();
				 wait();
				 calcall();
				 showall();
				 totals();
				 showtotals();
				 ready();
				 break;
			case 3 : cashissue();
				 wait();
				 calcall();
				 showall();
				 totals();
				 showtotals();
				 ready();
				 break;
			 case 4: switch( choice(3,resolutionchoice))
				 {
				   case 1: numpts = 10; break;
				   case 2: numpts = 15; break;
				   case 3: numpts = 30; break;
				 }
                                    break;
			 case 5:switch (choice(3,printchoice))
				  {
				     case 1: printdetails(); break;
				     case 2: demo1();break;
				     case 3: demo1(); break;
					   } break;
			 case 6: filedelete(); break;
			 case 7 : reinstall();
		         }
			 entercell(sys.cell);
			 break;

         case F09 : switch (sys.screen)
			{
			case SCREEN1 :
			    leavecell(sys.cell);
			    if (sys.display == HELDS) sys.display = DELTAS;
			    else sys.display = HELDS;
		    	if (sys.cell.row >= 8)
		   	 sys.cell.col = MONTH;
		    	textcolor(sys.graphics.colors.heading);
		    	textbackground(sys.graphics.colors.databack);
		    	drawheadings();
		    	showall();
			showtotals();
		    	entercell(sys.cell);break;
			case SCREEN2 : break;
			case SCREEN3 :
			    leavecell(sys.cell);
			    if (sys.display == INVVOL) sys.display = OVERVALUED;
			    else sys.display = INVVOL;
 		    	    textcolor(sys.graphics.colors.heading);
		    	    textbackground(sys.graphics.colors.databack);
		    	    drawheadings();
		    	    showall();
			    showtotals();
		    	    entercell(sys.cell);break;
		       }
		    break;
         case F10 : switch (choice(3,quitchoice))
		    {
		    case 0 : break;
		    case 2 : wait();
	   		    textcolor(LIGHTGRAY);
			    textbackground(BLACK);
			    savesys();
			    clrscr();
			    gotoxy(2,2);
			    cprintf(" The Option Analyst is a product of");
			    gotoxy(2,3);
			    cprintf(" EFAM RESOURCES Pty. Ltd.\n\r");
			    retcursor();
			    exit(0);
			    break;
                    case 1 : order(); clrscr(); genscreen();
			    break;
		    case 3 : break;
		    }
		    break;
	}
}





void movement()
{
char ch1,ch2;
entercell(sys.cell);

do {
	ch1 = getch();
	if (ch1 == 0)
	{
       	ch2 = getch();
        docommand(ch2);
	}
	else
	if (isalnum(ch1) || ch1 == 27 || ch1 == 8 || ch1 == '+' || ch1 == '-' || ch1 == '.')
	{
         ungetch(ch1);
	if ((sys.screen== SCREEN1) && ( status.data[sys.cell.row-8].month == -1) &&
	(sys.cell.col != MONTH) && (sys.cell.col <= HELDP))
	{
	leavecell(sys.cell);
	 clearrow();
	sys.cell.col = MONTH;
	entercell(sys.cell);
	}
	 ch2 = editcell(sys.cell);
	 if (ch2 == 0) ch2 = getch();
	 docommand(ch2);
	}
   } while (1);
}



void newfile()
{
sys.cell.row = 8;
sys.cell.col = MONTH;
sys.decimal = 2;
sys.screen = SCREEN1;
sys.display = HELDS;
clearall();
}




/* MAIN */

main()
{
strcpy(sys.usernames, startname);
savecursor();
textcolor(WHITE);
textbackground(BLACK);
clrscr();
registerfarbgidriver(Herc_driver_far);
registerfarbgifont(small_font_far);
newfile();
{
initialise();
if (sys.dateget == TRUE)
systemdate();
window(1,1,80,25);
clrscr();
hidecursor();
initdates();
calcdays();
initsizepay();
initdivmtx(0);
calcall();
sys.cell.row = 8;
sys.cell.col = MONTH;
sys.screen = SCREEN1;
page =  PAGEUP;
status.stockprice = 4.0;
totals();
if (status.stockprice != 3 + 1) exit(0);
sortstatus();
genscreen();
copyright();
demo();
loadfile("DEMO");
showall();
totals();
showtotals();
movement();}
retcursor();
}
