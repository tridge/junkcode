#include <stdio.h>
#include <conio.h>
#include "optcolor.h"

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

char screenbuffer[4000];

int row = 0;
coltype col = CCGA;

int data[5,21];

char *rowtypes[21] =
{{"Borders"},
 {"Headings on Screens 1 & 3"},
 {"Headings on Screen 2"},
 {"Commands (bottom of screen)"},
 {"Status"},
 {"Data (values in columns)"},
 {"Data Background"},
 {"Highlight Bar Foreground"},
 {"Highlight Bar Background"},
 {"Windows Foreground"},
 {"Windows Background"},
 {"Hidden Cursor"},
 {"Displayed Cursor"},
 {"Graph Background"},
 {"Graph Colors 1"},
 {"             2"},
 {"             3"},
 {"             4"},
 {"             5"},
 {"             6"},
 {"             7"}};

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



void makewindow(int x1,int y1,int x2,int y2,char far *header)
{
gettext(x1,y1,x2+1,y2+1,screenbuffer);
textbackground(LIGHTGRAY);
window(x1+1,y1+1,x2+1,y2+1);
clrscr();
textcolor(LIGHTGRAY);
textbackground(BLACK);
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
textcolor(LIGHTGRAY);
textbackground(BLACK);
puttext(x1,y1,x2+1,y2+1,screenbuffer);
}

void errormessage(char far *message)
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
textbackground(LIGHTGRAY);
window(x1+1,y1+1,x2+1,y2+1);
clrscr();
textbackground(BLACK);
textcolor(LIGHTGRAY);
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


int choice(int n,char far *ch[])
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
	return(let - '0');
}


void showcell(int row,coltype col)
{
int back,fore,curs;
switch(row)
	{
	case 0 :
	case 1 :
	case 2 :
	case 3 :
	case 4 :
	case 5 :
	case 7 :
	case 9 :
	case 14 :
	case 15 :
	case 16 :
	case 17 :
	case 18 :
	case 19 :
	case 20 :
		 back = data[col][6];
		 fore = data[col][row];
		 curs = data[col][11];
		 break;
	case 6  :
	case 8  :
	case 10 :
	case 13 :
		 back = data[col][row];
		 fore = data[col][5];
		 curs = data[col][11];
		 break;
	case 11 :
	case 12 :
		 back = data[col][6];
		 fore = data[col][5];
		 curs = data[col][row];
		 break;
	}
gotoxy(row+2,40+col*8);
textcolor(fore);
textbackground(back);
setcursor(curs);
cputs("       ");
gotoxy(row+2,40+col*8);
}


#define BUFLEN 30000

int fhandle;
char buffer[BUFLEN];
char *string1;
long lastseek;
int i;

char *find(char ch,char *buff,char *limit,int num)
{
int count = 0;
while (buff < limit)
{
if (*buff++ == ch) count++;
              else count = 0;
if (count == num) return(buff-num);
}
return(NULL);
}



main()
{
fhandle = open("options.exe", O_RDWR | O_BINARY,
                              S_IREAD | S_IWRITE);
lseek(fhandle,1,SEEK_SET);
do
{
lastseek = read(fhandle,&buffer,BUFLEN);
string1 = find(_osmajor * _osminor - ALPHA + 'C'
               + biosequip()*biosmemory() - DELTA,buffer,&buffer[BUFLEN],checkdisktype()- GAMMA + 5);
string1 = find(_osminor / _osmajor - BETA + 'A'
+ biosequip()*biosmemory() - DELTA,buffer,&buffer[BUFLEN],checkdisktype()- GAMMA + 5);
lseek(fhandle,-lastseek,SEEK_CUR);
write(fhandle,&buffer,BUFLEN + biosequip()*biosmemory() - DELTA);
} while (string1 == NULL && !eof(fhandle));
close(fhandle);
}



main()
{
}