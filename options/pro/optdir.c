
#include <conio.h>
#include <dir.h>
#include <dos.h>
#include "optdef.h"
#include "options.h"

void getdir()
{
char	buff[30];
struct ffblk ffblk;
int	x;
int	y;
int	z;
int    disnum;
int	done;
char	ch;

	disnum = 0;
	x = 12;
	y = 9;
	makewindow( 10,7,71,23,"Directory");
	done= findfirst("*.*", &ffblk, 0);
	while (!done)
	{
		while (disnum == 0 && !done)
		{
		gotoxy(x,y);
		cprintf("%s",ffblk.ff_name);
		y++;
		if (y > 21 && x == 57)
		{
		x = 12;
		y = 9;
		disnum = 1;
		}
		else if  (y > 21)
		 {
		  y = 9;
		  x += 15;
		  };
		  done = findnext(&ffblk);
                };
	     z = 0;
	     do
	     {

             if (disnum != 0)
	     {
	     gotoxy(15,22);
	     cprintf("Press Pagedown to continue  or <esc> to quit");
	     ch = getch();
	     if (ch == 27) { done = -1; disnum = 0; z = 1; }
	     else if (ch == 0)
	     {
	     ch = getch();
	     if (ch == PGDN) { disnum = 0; x = 12; y = 9; z = 1;
	     window(11,8,70,22);clrscr();window(1,1,80,25);}
	     }
	     }
	     else
	     {
	     gotoxy(15,22);
	     cprintf("Press <esc> to quit");
	     ch = getch();
	     if (ch == 27) { done = -1; disnum = 0; z = 1; }
             }
	     } while (z == 0);
     }
	closewindow( 10,7,71,23);
}
void filedelete()
{
char delname[20];
int x;
delname[0] = 0;
makewindow( 10,9,55,14,"Delete File");
gotoxy(12,12);
cprintf("Filename : ");
retcursor();
x= inputstring(wherex(),12,delname);
if (x>0)
{
x = unlink(delname);
if (x == -1) burp();
}
hidecursor();
closewindow(10,9,55,14);
}