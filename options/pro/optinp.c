#include <fcntl.h>
#include <io.h>
#include <dir.h>
#include <sys\stat.h>
#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <stdio.h>
#include <graphics.h>
#include <ctype.h>
#include <math.h>

#include "optdef.h"
#include "options.h"

/* INPUT FUNCTIONS FOR OPTIONS.C  18/5/88 */
extern int mapedready;
extern int tosavesys;

static char *setupchoice[5] =
{{"Setup Menu"},
 {"Graphics Card :             "},
 {"Screen        :             "},
 {"Time Clock    :             "},
 {"Printer       :             "}
 };
 static char *gcardchoice[5] =
 {{"Graphics card"},
 {"Automatic"},
 {"Hercules"},
 {"CGA"},
 {"EGA or VGA"}
 };
 static char *screenchoice[3] =
 {{"Screen Type"},
 {"Colour"},
 {"Monochrome"}
 };
 static char *printerchoice[3] =
 {{"Dotmatrix Printer"},
 {"EPSON Compatible"},
 {"IBM Compatible"}
 };
 static char *timeclchoice[3] =
 {{"Timeclock Installed"},
 {"YES"},{"NO"}};

void savesys()
{
char  ch;
char error[60];
int fhandle;
        if (driveready() == TRUE)
{
	fhandle = open("def.sys", O_RDWR | O_CREAT | O_TRUNC | O_BINARY,
                                  S_IREAD | S_IWRITE);
	if (fhandle != -1)
	{
        if (write(fhandle,&sys,sizeof(systype)) < sizeof(systype))
	errormessage("Disk access error\n\rDisk probably full");
	else
	strncpy(sys.lastfile, filename, 15);
	close(fhandle);
	}
	else
	{
	sprintf(error,"Disk access error\n\r%s",sys_errlist[errno]);
	errormessage(error);
	}
} else errormessage("Drive not ready\n\rCheck hardware");
tosavesys = FALSE;
strncpy(filename,sys.lastfile, 15);
}

int recallsys()
{
char namecheck[20];
char error[60];
char  ch;
int fhandle;
struct dfree  dfreep;
int	filesize;
if (driveready()== TRUE)
 {
      fhandle = open("def.sys",O_RDWR | O_BINARY);
      if (fhandle != -1)
      {
      read(fhandle,namecheck,20);
      if (strncmp(namecheck,"OPTIONS SYSTEM V1.0",20) != 0)
      {
      errormessage("Invalid system file\n\rPossibly corrupted");
      close(fhandle);
      return(FALSE);
      }
      else
      {
	lseek(fhandle,0,0);
        read(fhandle,&sys,sizeof(systype));
	close(fhandle);
	return(TRUE);
      }
      }
      else
	{
        return(FALSE);
        }
 } else errormessage("Drive not ready\n\rCheck hardware");
	return(FALSE);
}

void setuparray()
{
if (sys.graphics.name[0] == 'H')
   strcpy(setupchoice[1],"Graphics Card : HERCULES    ");
   else if (sys.graphics.name[1] == 'E')
    strcpy(setupchoice[1],"Graphics Card : EGA/VGA     ");
       else if (sys.graphics.name[1] == 'C')
    strcpy(setupchoice[1],"Graphics Card : CGA         ");
    if (sys.graphics.name[0] == 'C')
    strcpy(setupchoice[2],"Screen        : COLOUR      ");
    else
    strcpy(setupchoice[2],"Screen        : MONOCHROME  ");
    if (sys.dateget == TRUE)
    strcpy(setupchoice[3],"Time Clock    : YES         ");
    else
    strcpy(setupchoice[3],"Time Clock    : NO          ");
    if (sys.prt == 0)
    strcpy(setupchoice[4],"Printer       : EPSON       ");
    else if (sys.prt == 1)
    strcpy(setupchoice[4],"Printer       : IBM         ");

 };
void reinstall()
{
int done = FALSE;
int gmode,gdriver;
do
{     setuparray();
      switch ( choice(4,setupchoice) )
        {
	 case 1 : switch (choice (4,gcardchoice))
		     {
		      case 1: detectgraph(&gdriver,&gmode); break;
		      case 2: gdriver = HERCMONO; break;
		      case 3: gdriver = CGA; break;
		      case 4: gdriver = EGA; break;
		      }
		      mapedready = FALSE;
            		switch (gdriver)
		      {	case CGA : 	if ( sys.graphics.name[0] == 'C' )
 					sys.graphics = ccgagraphics;
					else
					sys.graphics = mcgagraphics;
					break;
			case EGA :     if ( sys.graphics.name[0] == 'C' )
 					sys.graphics = cegagraphics;
					else
					sys.graphics = megagraphics;
					break;
			case EGAMONO : sys.graphics = megagraphics; break;
			case EGA64 : sys.graphics = cegagraphics; break;
                        case VGA : if ( sys.graphics.name[0] == 'C' )
 					sys.graphics = cegagraphics;
					else
					sys.graphics = megagraphics;
					break;
			case HERCMONO : sys.graphics = hercgraphics;break;
      			     } break;
	case 2 : switch (choice(2,screenchoice))
		    {
		    case 1: if (sys.graphics.name[0] != 'H')
		              { if (sys.graphics.name[1] == 'E')
                         	sys.graphics = cegagraphics;
				else if (sys.graphics.name[1] == 'C')
					sys.graphics = ccgagraphics;
			      } break;
 		    case 2:  if (sys.graphics.name[0] != 'H')
		              { if (sys.graphics.name[1] == 'E')
                         	sys.graphics = megagraphics;
				else if (sys.graphics.name[1] == 'C')
					sys.graphics = mcgagraphics;
		              }
		      } mapedready = FALSE; break;
	case 3 : switch (choice(2,timeclchoice))
		   {
		   case 1: sys.dateget = TRUE; break;
		   case 2: sys.dateget = FALSE; break;
		    } break;
	case 4 : switch (choice(2,printerchoice))
		  {
		  case 1: sys.prt = 0; break;
		  case 2: sys.prt = 1; break;
		  } break;
	default : done = TRUE; break;
	}
 } while (!done);
savesys();
genscreen();
}

void initialise()
{
char  ch;
char m1,c2;
int gmode,gdriver;
sys.graphics = mcgagraphics;
if (!recallsys())
{
      clrscr();
      gotoxy(5,3);
      cprintf("WELCOME TO THE OPTION MARKET ANALYST");
      gotoxy(5,4);
      cprintf( "    INITIALISATION PROGRAM");
      gotoxy(7,6);
      cprintf(" Press a number key to choose the following");

	gotoxy(5,8);
	cprintf(" What type of screen do you have?");

	gotoxy(5,10);
	cprintf("1  Monochrome");
	gotoxy(5,12);
	cprintf("2  Colour");
	c2 = getch();
	while (c2 != '1' && c2 != '2')
        c2 = getch();
	clrscr();
	gotoxy(5,3);
	cprintf( "What type of graphics card do you have? ");
	gotoxy(5,12);
	cprintf("1  Automatic selection");
	gotoxy(5,14);
	cprintf("2  CGA");
		gotoxy(5,16);
	cprintf("3  Hercules");
		gotoxy(5,18);
	cprintf("4  EGA");
		gotoxy(5,20);
	cprintf("5  VGA");
	ch = getch();
	while (!(ch <= '5' && ch  >= '1'))
	{
	ch = getch();
	}
	       switch (ch)
	       {
	       case '1' :
		detectgraph(&gdriver,&gmode);
		break;
		case '2' : gdriver = CGA; break;
		case '3' : gdriver = HERCMONO; break;
		case '4' : gdriver = EGA; break;
		case '5' : gdriver = VGA; break;
		}

		switch (gdriver)
		{	case CGA : 	if ( c2 == '2' )
 					sys.graphics = ccgagraphics;
					else
					sys.graphics = mcgagraphics;
					break;
			case EGA :     if ( c2 == '2' )
 					sys.graphics = cegagraphics;
					else
					sys.graphics = megagraphics;
					break;
			case EGAMONO : sys.graphics = megagraphics; break;
			case EGA64 : sys.graphics = cegagraphics; break;
                        case VGA : if ( c2 == '2' )
 					sys.graphics = cegagraphics;
					else
					sys.graphics = megagraphics;
					break;
			case HERCMONO : sys.graphics = hercgraphics;break;
			default : sys.graphics = hercgraphics;
				  errormessage("Graphics hardware not supported");
				  break;
		}
  clrscr();
  gotoxy( 5,5);
  cprintf(" Is this todays date ");
  showsysdate();
  gotoxy(5,8);
  cprintf("1 YES");
  gotoxy(5,10);
  cprintf("2 NO");
  ch = getch();
	while (ch != '1' && ch != '2')
        ch = getch();
	if ( ch == '2' )
	sys.dateget = FALSE;
	else
	sys.dateget = TRUE;
  clrscr();
  gotoxy( 5,5);
  cprintf(" EPSON or IBM compatible printer?");
  gotoxy(5,8);
  cprintf("1 EPSON");
  gotoxy(5,10);
  cprintf("2 IBM");
  ch = getch();
	while (ch != '1' && ch != '2')
        ch = getch();
	if ( ch == '2' )
	sys.prt = 1;
	else
	sys.prt = 0;
        initdates();
	calcdays();
	initsizepay();
	initdivmtx(0);
	savesys();
	clearall();
}
}



void loadfile(char *fname)
 {
char checkname[20];
char error[60];
int fhandle;

if (driveready() == TRUE)
{
	fhandle = open(fname,O_RDWR | O_BINARY);
	if (fhandle != -1)
	{
	read(fhandle,checkname,20);
	checkname[19] = 0;
	if (strncmp(checkname,"OPTIONS ANALYST V1.0",19) == 0)
  	{
		lseek(fhandle,0,0);
	        read(fhandle, &status,sizeof(statustype));
		sys.cell.row = 3;
		sys.cell.col = SHAREPRICE;
		sys.screen = SCREEN1;
		sys.display = HELDS;
	        page = PAGEUP;
		recalcvolvalues = TRUE;
		strncpy(sys.lastfile,filename,15);
  	} else
	errormessage("Invalid file\n\rPossibly corrupted");
        close(fhandle);
	}
	else
	{
	sprintf(error,"Disk access error\n\r%s",sys_errlist[errno]);
	errormessage(error);
	}
} else errormessage("Drive not ready\n\rCheck hardware");
strncpy(filename,sys.lastfile,15);
}



void systemdate()
{
int	x;
int	y;
struct date today;
getdate(&today);
sys.date[0]=  '0' + (0.1 * today.da_day );
sys.date[1]=  '0' + ( today.da_day -(10 *(sys.date[0]-'0')));
sys.date[2]= months[today.da_mon -1][0];
sys.date[3]= months[today.da_mon -1][1];
sys.date[4]= months[today.da_mon -1][2];
if (today.da_year > 1999)
{
x = (today.da_year-2000)/10;
y = (today.da_year-2000) - x*10;
}
else
{
x=  (today.da_year-1900)/10;
y = (today.da_year-1900) - x*10;
}

sys.date[5]= '0' + x;
sys.date[6]= '0' + y;
/*cprintf("%d/%d/%d", today.da_day, today.da_mon,today.da_year);*/

}

void showsysdate()
 {
struct date today;
getdate(&today);
cprintf("%d/%d/%d", today.da_day, today.da_mon,today.da_year);
}


int findmonth(char *month)
{
int index;
index = 0;
while (index <12 && strncmp(month,months[index],3) != 0)
index++;
if (index == 12) return(-1);
else return(index);
}

void bonusissue()
{
int x;
int y;
char ch;
int count;
	makewindow(10,5,55,15, "Bonus Issue");
	gotoxy( 12, 8);
	cprintf(" A bonus issue of  X  shares ");
	gotoxy(12,9);
	cprintf(" for every  Y  shares held");
	gotoxy(15,10);
	cprintf("X =     ");
	retcursor();
        x=	inputinteger(23,10,0,3);
	ch = getch();
	if (ch == 27)   ungetch(ch);
	else
	{
	if (ch == 0) ch = getch();
	ch = ch +1;
	gotoxy(15,12);
	cprintf("Y =     ");
	y = inputinteger( 23,12, 0,3);
	if (y >0)
	{
          for (count = 0; count < 30; count++)
	   {

	    status.data[count].strike  = (  status.data[count].strike * y) /(x + y);
	    }
	   for (count = 0; count <= 15; count++)
	   {

	   status.sizepay[count].sharesper =
	   ( status.sizepay[count].sharesper * (x + y))/y;
	   }
           status.stockheld =
	   ( status.stockheld * (x + y))/y;
	 }
       } /* else */
	   closewindow(10,5,55,15);
	   hidecursor();
}


void cashissue()
{
int x;
int y;
double z;
char ch;
int count;
	makewindow(10,5,55,15, "Cash Issue");
	gotoxy( 12, 8);
	cprintf(" A Cash issue of  X cents per share ");
	gotoxy(15,10);
	cprintf("X =     ");
	retcursor();
        x=	inputinteger(23,10,0,3);
	z = 0.01 * x;
          for (count = 0; count < 30; count++)
	   {
	   if (status.data[count].month != -1 &&
	       status.data[count].strike > z)
	       status.data[count].strike = status.data[count].strike - z;
	       else
	       status.data[count].strike = 0;
	    }

	   closewindow(10,5,55,15);
	   hidecursor();
}



int legaldate(char *date)
{
int legal = TRUE;
int day;
int month;
int year;
convertdate(date,&year,&month,&day);
if (month < 0 || month > 11) legal = FALSE;
else
if (year < 40 || year > 99)  legal = FALSE;
else
if (day < 1 || day > 31) legal = FALSE;
else
switch (month+1)
{       case 9 :
	case 6 :
	case 4 :
	case 11: if (day > 30) legal = FALSE; break;
	case 2 : if ((int)(year/4)*4 == year)
		 {if (day > 29) legal = FALSE;}
		 else if (day > 28) legal = FALSE; break;
	default : if (day > 31) legal = FALSE; break;
}
return(legal);
}


int inputstring(int x,int y,char *string)
{
char ch;
int firstch = TRUE;
char instring[20];
strcpy(instring,string);
do
{
gotoxy(x,y);
cprintf("%s",instring);
ch = getch();
if (isalpha(ch)) ch= toupper(ch);
if (firstch == TRUE && (isalnum(ch)))
{
gotoxy(x,y);
cprintf("                    ");
instring[0] = ch;
instring[1] = 0;
gotoxy(x,y);
cprintf("%s",instring);
ch = getch();
if (isalpha(ch)) ch= toupper(ch);
}
if ((isalnum(ch) || ch == '.')
   && strlen(instring) < 13)
	{
	 instring[strlen(instring)+1] = 0;
	 instring[strlen(instring)] = ch;
	}
else
if (ch == 8 && strlen(instring) > 0)
{
     instring[strlen(instring)-1] = 0;
     gotoxy(x,y);
     cprintf("                ");
}
firstch = FALSE;
if (ch == 0) getch();
} while (ch != 13 && ch != 27);
if (ch == 27)
{
return(-1);
}
else
{
strncpy(string,instring,15);
return(1);
}
}


int inputmonth(int x,int y,int month)
{
char ch;
int index;
char monthstr[3];
int firstch = TRUE;
char instring[10];
strncpy(instring,months[month],3);
instring[3] = 0;
do
{
gotoxy(x-strlen(instring)-3,y);
cprintf("  %s",instring);
ch = getch();
if (ch == '+')
	{index = findmonth(instring);
         if (index >= 0 && index < 11)
	 index++;
	 else if (index == 11 || index == -1)
	 index = 0;
	 strncpy(instring,months[index],3);}
else
if (ch == '-')
	{index = findmonth(instring);
         if (index > 0 && index <= 11)
	 index--;
	 else if (index == 0 || index == -1)
	 index = 11;
	 strncpy(instring,months[index],3);}
else
if (firstch && isalpha(ch))
{
ch = toupper(ch);
instring[0] = ch;
instring[1] = 0;
}
else
if (isalpha(ch) && strlen(instring) < 3)
	{
         ch = toupper(ch);
	 instring[strlen(instring)+1] = 0;
	 instring[strlen(instring)] = ch;
	}
else
if (ch == 8 && strlen(instring) > 0)
         instring[strlen(instring)-1] = 0;
if (ch == 27)
{
strncpy(instring,months[month],3);
instring[3] = 0;
firstch = TRUE;
}
else
firstch = FALSE;
} while (ch != 0 && ch != 13);
if (findmonth(instring) != -1)
month = findmonth(instring);
ungetch(ch);
return(month);
}





int inputinteger(int x,int y,int defnum,int max)
{
char instring[20];
char ch;
int count;
int num;
int firstch = TRUE;

gotoxy(x-max,y);
for(count=1;count<=max;count++) putch(' ');
if (defnum == 0)
instring[0] = 0;
else
sprintf(instring,"%-d",defnum);
while (instring[strlen(instring)-1] == ' ')
instring[strlen(instring)-1] = 0;
gotoxy(x-strlen(instring),y);
cprintf("%s",instring);
do
{       ch = getch();
	if (firstch && (isdigit(ch) || ch == '-'))
		{
                gotoxy(x-strlen(instring),y);
	        for(count=1;count<=strlen(instring);count++)
                putch(' ');
		instring[1] = 0; instring[0] = ch;
                gotoxy(x-strlen(instring),y);
  		cprintf("%s",instring);
		}
	else
	if ((isdigit(ch) || (strlen(instring) == 0 && ch == '-'))
			 && strlen(instring) < max)
	{instring[strlen(instring)+1] = 0;
	 instring[strlen(instring)] = ch;
	 gotoxy(x-strlen(instring),y);
	 cprintf("%s",instring);
	}
	else
	if (ch == 8 && strlen(instring) > 0)
	{gotoxy(x-strlen(instring),y);
         instring[strlen(instring)-1] = 0;
	 cprintf(" %s",instring);
        }
	if (ch == 27)
	{
	gotoxy(x-max,y);
	for(count=1;count<=max;count++) putch(' ');
	if (defnum == 0)
	instring[0] = 0;
	else
	sprintf(instring,"%-d",defnum);
	while (instring[strlen(instring)-1] == ' ')
	instring[strlen(instring)-1] = 0;
	gotoxy(x-strlen(instring),y);
	cprintf("%s",instring);
	firstch = TRUE;
	}
	else
	firstch = FALSE;
} while (!(ch == 13 || ch == 0));
if (strlen(instring) != 0)
num = atoi(instring);
else num = defnum;
ungetch(ch);
return(num);
}


long inputlint(int x,int y,long *defnum,int max)
{
char instring[20];
char ch;
int count;
long num;
int firstch = TRUE;

gotoxy(x-max,y);
for(count=1;count<=max;count++) putch(' ');
if (defnum == 0)
instring[0] = 0;
else
ltoa(*defnum,instring,10);
while (instring[strlen(instring)-1] == ' ')
instring[strlen(instring)-1] = 0;
gotoxy(x-strlen(instring),y);
cprintf("%s",instring);
 while(wherex() <= endcell(sys.cell.col)) putch(' ');
do
{       ch = getch();
	if (firstch && (isdigit(ch) || ch == '-'))
		{
                gotoxy(x-strlen(instring),y);
	        for(count=1;count<=strlen(instring);count++)
                putch(' ');
		instring[1] = 0; instring[0] = ch;
                gotoxy(x-strlen(instring),y);
  		cprintf("%s",instring);
		}
	else
	if ((isdigit(ch) || (strlen(instring) == 0 && ch == '-'))
			 && strlen(instring) < max)
	{instring[strlen(instring)+1] = 0;
	 instring[strlen(instring)] = ch;
	 gotoxy(x-strlen(instring),y);
	 cprintf("%s",instring);
	}
	else
	if (ch == 8 && strlen(instring) > 0)
	{gotoxy(x-strlen(instring),y);
         instring[strlen(instring)-1] = 0;
	 cprintf(" %s",instring);
        }
	if (ch == 27)
	{
	gotoxy(x-max,y);
	for(count=1;count<=max;count++) putch(' ');
	if (defnum == 0)
	instring[0] = 0;
	else
	ltoa(*defnum,instring,10);
	while (instring[strlen(instring)-1] == ' ')
	instring[strlen(instring)-1] = 0;
	gotoxy(x-strlen(instring),y);
	cprintf("%s",instring);
	firstch = TRUE;
	}
	else
	firstch = FALSE;
} while (!(ch == 13 || ch == 0));
if (strlen(instring) != 0)
status.stockheld = atol(instring);
else num = *defnum;
ungetch(ch);
return(num);
}



double inputreal(int x,int y,double defnum,int max,int dec)
{
char instring[20];
char ch;
int count;
double num;
int firstch = TRUE;
gotoxy(x-max,y);
for(count=1;count<=max;count++) putch(' ');
if (defnum == 0)
instring[0] = 0;
else
sprintf(instring,"%-8.*f",dec,defnum);
while (instring[strlen(instring)-1] == ' ')
instring[strlen(instring)-1] = 0;
gotoxy(x-strlen(instring),y);
cprintf("%s",instring);
do
{       ch = getch();

	if (firstch && (isdigit(ch) || ch == '.'))
		{
                gotoxy(x-strlen(instring),y);
	        for(count=1;count<=strlen(instring);count++)
                putch(' ');
		if (ch != '.')
		{
		instring[1] = 0; instring[0] = ch;
		}
		else
		{
		instring[0] = '0';
		instring[1] = ch;
		instring[2] = 0;
		}
		gotoxy(x-strlen(instring),y);
  		cprintf("%s",instring);
		}
	else
	if (isdigit(ch)
			 && (atof(instring) < pow(10,max-4)
			 || strchr(instring,'.') != NULL)
			 && strlen(instring) < max)
	{instring[strlen(instring)+1] = 0;
	 instring[strlen(instring)] = ch;
	 gotoxy(x-strlen(instring),y);
	 cprintf("%s",instring);
	}
	else
	if (ch == '.' && strchr(instring,'.') == NULL)
        {instring[strlen(instring)+1] = 0;
	 instring[strlen(instring)] = ch;
	 gotoxy(x-strlen(instring),y);
	 cprintf("%s",instring);
	}
	else
	if (ch == 8 && strlen(instring) > 0)
	{gotoxy(x-strlen(instring),y);
         instring[strlen(instring)-1] = 0;
	 cprintf(" %s",instring);
        }
	if (ch == 27)
	{gotoxy(x-strlen(instring),y);
	 for(count=1;count<=strlen(instring);count++)
	 putch(' ');
	 sprintf(instring,"%-8.*f",dec,defnum);
	 while (instring[strlen(instring)-1] == ' ')
	 instring[strlen(instring)-1] = 0;
	 gotoxy(x-strlen(instring),y);
	 cprintf("%s",instring);
	 firstch = TRUE;
	}
	else
	firstch = FALSE;
} while (!(ch == 13 || ch == 0));
if (strlen(instring) != 0)
num = atof(instring);
else num = defnum;
ungetch(ch);
return(num);
}


int inputdate(int x,int y,char *date)
{
char instring[20];
char ch;
int count;
int max = 7;
int firstch = TRUE;

gotoxy(x-max,y);
for(count=1;count<=max;count++) putch(' ');
strncpy(instring,date,7);
instring[7]=0;
gotoxy(x-strlen(instring),y);
cprintf("%s",instring);
do
{
do
{       ch = getch();
if (firstch && isdigit(ch))
	{gotoxy(x-strlen(instring),y);
	 for(count=1;count<=strlen(instring);count++)
	 putch(' ');
	 instring[1] = 0;
	 instring[0] = ch;
	 gotoxy(x-strlen(instring),y);
	 cprintf("%s",instring);}
else
switch (strlen(instring))
{	case 0 :
	case 1 :
	case 5 :
	case 6 : if (isdigit(ch))
		{instring[strlen(instring)+1] = 0;
		 instring[strlen(instring)] = ch;
		 gotoxy(x-strlen(instring),y);
		 cprintf("%s",instring);}
		 break;
	case 2 :
	case 3 :
	case 4 : if (isalpha(ch))
		{instring[strlen(instring)+1] = 0;
		 instring[strlen(instring)] = toupper(ch);
		 gotoxy(x-strlen(instring),y);
		 cprintf("%s",instring);}
		 break;
}
	if (ch == 8 && strlen(instring) > 0)
	{gotoxy(x-strlen(instring),y);
         instring[strlen(instring)-1] = 0;
	 cprintf(" %s",instring);
        }
	if (ch == 27)
	{gotoxy(x-strlen(instring),y);
	 for(count=1;count<=strlen(instring) +1;count++)
	 putch(' ');
	 strncpy(instring,date,7);
	 instring[7]=0;
	 gotoxy(x-strlen(instring),y);
	 cprintf("%s",instring);
	 firstch = TRUE;
	}
	else
	firstch = FALSE;
} while (!(ch == 13 || ch == 0));
if (strlen(instring) == 7)
strcpy(date,instring);
if (legaldate(date) == FALSE) errormessage("Illegal date");
} while (legaldate(date) == FALSE);
ungetch(ch);
}







