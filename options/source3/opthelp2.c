
#include <string.h>
#include <conio.h>
#include "optdef.h"
#include "options.h"

/* help module for options analyst 27/5/88 */


static char *layout_1 =
"   The Options Analyst is divided into three screens. Each\n"
"of these screens may be accessed by selecting F6 and then\n"
"choosing appropriately from the menu.                   \n"
"                                                        \n"
"   SCREEN 1 provides theoretical valuation of individual\n"
"options and whole positions in a stock.                 \n"
"                                                        \n"
"   SCREEN 2 allows the user to enter data on the expiry \n"
"date of each month and specify any dividends that may be\n"
"given on the stock.                                     \n"
"                                                        \n"
"   SCREEN 3 provides volatility inversion and compares  \n"
"actual market prices to the theoretical values.         \n"
"                                          PRESS ANY KEY ";


static char *layout_2 =
"   The two middle columns of this screen show the       \n"
"theoretical values of the options in the left columns. \n"
"                                                        \n"
"   The two columns on the right of the screen may be used\n"
"either to show the theoretical delta of each corresponding\n"
"option or to enter a position in the market for valuation.\n"
"The F9 key is used to toggle between these uses.\n"
"                                                        \n"
"   The total call and put positions are shown at the    \n"
"bottom of the two right columns and the total theoretical\n"
"value/delta of the position is shown above these columns.\n"
"                                                        \n"
"   Two pages of options are available with PgUp/PgDown. \n"
"                                          PRESS ANY KEY ";


static char *layout_3 =
"   This screen is used to enter information about\n"
"dividends and expiry dates. The dividend day and amount in\n"
"cents is entered in the right two columns corresponding to\n"
"the month of the dividend.\n"
"\n"
"   The multiplier column is used for entering the\n"
"equivalent number of shares for each option contract.\n"
"\n"
"   The number of days between the current date and the\n"
"expiration date for each month is displayed in the Days-\n"
"Left column. Ensure the expiry day is correct for each\n"
"month.\n"
"                                          PRESS ANY KEY ";


static char *layout_4 =
"   This screen is used to calculate the implied volatility\n"
"corresponding to actual market prices. The market prices\n"
"are entered in the right two columns and the corresponding\n"
"implied volatilities are shown in the centre two columns.\n"
"\n"
"   The weighted put and call volatilities are shown at the\n"
"bottom of these two columns and the total weighted\n"
"volatility is shown in the top right corner.\n"
"\n"
"   By pressing F9 the display will toggle to showing the\n"
"percentage by which the theoretical values of Screen 1\n"
"differ from the market values. The display will still show\n"
"the weighted volatilities as before.\n"
"                                          PRESS ANY KEY ";

static char *entering_1 =
"   Movement around the screens is achieved by using the\n"
"four cursor keys normally located on the right hand side\n"
"of the keyboard. As you move into each cell it will be\n"
"highlighted.\n"
"\n"
"   The highlight bar will wrap around if you reach the top\n"
"of the screen and will start again from the bottom. \n"
"\n"
"   A second page of cells is available on screen 1 and\n"
"screen 3 using the PageUp and PageDown keys.\n"
"\n"
"   If you find that the cursor will not move correctly\n"
"check that the NumLock key is turned off.\n"
"                                          PRESS ANY KEY ";

static char *entering_2 =
"   The program will recalculate any values being currently\n"
"displayed as soon as any change has been made to the\n"
"values entered. The mathematical calculations required are\n"
"extremely complex and may take some time on an ordinary\n"
"PC.\n"
"\n"
"   If you find that the program is too slow at\n"
"recalculating values then we advise you to install a\n"
"mathematical co-processor in your machine. Speed increases\n"
"of up to twenty times have been noted with some machines\n"
"when a mathematical co-processor has been installed. The\n"
"program will automatically detect the presence of a co-\n"
"processor and use it for all mathematical operations.\n"
"                                          PRESS ANY KEY ";

static char *entering_3 =
"   To edit a cell first move the highlight bar into that\n"
"cell using the cursor keys. Values may then be entered\n"
"using the number keys at the top of the keyboard or using\n"
"the numeric keypad.\n"
"\n"
"   The date is entered in a similar fashion and should be\n"
"of the form DDMMMYY. (eg. 01JAN80)\n"
"\n"
"   Months and entered either by typing three letters (eg.\n"
"JAN) or by pressing the plus or minus keys to roll through\n"
"the months of the year. Pressing the plus key on an empty\n"
"month cell starts by entering the month of the cell above.\n"
"\n"
"                                          PRESS ANY KEY ";

static char *keys_1 =
"   This function key displays the help system that you are\n"
"currently using.\n"
"\n"
"   To escape from the help system and continue with the\n"
"program where you left off use the key marked <Esc>.\n"
"\n"
"   For more complete information on the program refer to\n"
"the users manual. If you still have any questions please\n"
"contact the distributor.\n"
"\n"
"\n"
"\n"
"\n"
"                                          PRESS ANY KEY ";

static char *keys_2 =
"   This command will save the current position in the\n"
"default directory after prompting you for a name for the\n"
"file.\n"
"\n"
"   The file name may be any normal DOS file name including\n"
"extension. Refer to your DOS manual for information on\n"
"legal file names.\n"
"\n"
"   The current file being used is displayed near the top\n"
"left corner of the screen.\n"
"\n"
"\n"
"\n"
"                                          PRESS ANY KEY ";

static char *keys_3 =
"   This comand will recall a file from the default\n"
"directory after prompting you for the name of the file. \n"
"\n"
"   If you cannot remember the names of the files you have\n"
"saved use the DIRECTORY command in the Utilities function.\n"
"\n"
"   The current file being used is displayed near the top\n"
"left corner of the screen.\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"                                          PRESS ANY KEY ";

static char *keys_4 =
"   The F4 function key will sort the options currently\n"
"entered into the program in the traditional order for\n"
"options.\n"
"\n"
"   The options will be sorted first by month and secondly\n"
"by strike.\n"
"\n"
"   This allows additional strikes to be appended to the\n"
"end of the list and then sorted into their correct\n"
"position with one key.\n"
"\n"
"\n"
"\n"
"                                          PRESS ANY KEY ";

static char *keys_5 =
"   The F5 function key shows the graphing menu and allows\n"
"many combinations of option positions to be graphed.\n"
"\n"
"   A `Whole Position' graph will show a graph showing the\n"
"sum total of all the options and stock held in that stock.\n"
"The `This Option' graphs will show the position for the\n"
"call and put options held for the currently highlighted\n"
"row. An error message will appear if no position is held. \n"
"\n"
"   The previous `Whole Position' graphs are stored and can\n"
"be accessed using the `Previous' choices from the menu.\n"
"This saves recalculation of graphs which can take some\n"
"time due to the complex mathematics involved.\n"
"                                          PRESS ANY KEY ";

static char *keys_6 =
"   The F6 key shows the menu allowing the user to select\n"
"which of the three main screens is active.\n"
"\n"
"   For more information on the function of the three\n"
"screens see the program layout section in this help\n"
"system.\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"                                          PRESS ANY KEY ";

static char *keys_7 =
"   The F7 key allows the user to quickly erase sections of\n"
"the entered data without individually blanking the cells.\n"
"\n"
"   The `Clear row' command will erase the row  that the\n"
"cursor is currently positioned at. Similarly the `Clear\n"
"Column' function will clear the column in which the cursor\n"
"is currently positioned. These functions will no work in\n"
"the Month or Multiplier columns or in the cells at the top\n"
"of the screens. The `Clear All' function will return the\n"
"program to the status it had when the program was called,\n"
"clearing all the entered data. \n"
"\n"
"   The data cannot be recovered once cleared.\n"
"                                          PRESS ANY KEY ";

static char *keys_8 =
"   The F8 key provides several useful utilities. The user\n"
"manual should be consulted to obtain more information on\n"
"the use of each of these functions.\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"                                          PRESS ANY KEY ";

static char *keys_9 =
"   The F9 key provides a toggle between the two types of\n"
"display for screen 1 and screen 3.\n"
"\n"
"   On screen 1 the display will switch between displaying\n"
"the currently held position and the theoretical Delta of\n"
"the position.\n"
"\n"
"   On screen 3 the display will toggle between the implied\n"
"volatilities, calculated from the market values entered by\n"
"the user, and the percentage by which the market values\n"
"are different from the theoretical values of screen 1.\n"
"\n"
"   The displays are toggled back by again pressing F9.\n"
"                                          PRESS ANY KEY ";

static char *keys_10 =
"   The F10 key exits the program and returns the user to\n"
"DOS.\n"
"\n"
"   The program will display a message asking the user if\n"
"he wishes to save before quitting. If you do not save to\n"
"disk then any changes you have made will be lost.\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"                                          PRESS ANY KEY ";

static char *graphing_1 =
"   Price graphs show how the value of an option position\n"
"will change with changes in the underlying stock price.\n"
"\n"
"   The `Current' graph shows how the position would change\n"
"at the current time and the `Expiry' graph shows how the\n"
"position would change at the first expiry date of the\n"
"options currently held. \n"
"\n"
"   A vertical line on the graphs can be used to obtain\n"
"exact values and is moved with the cursor or tab keys.\n"
"The range of the Shareprice used depends on your choice\n"
"of Zoom factor. The graph may be redrawn by pressing 'R'\n"
"or printed (Compatible graphics printer) using `P'\n"
"                                          PRESS ANY KEY ";

static char *graphing_2 =
"   The time graphs shows the decay or growth of an option\n"
"position with time with the stock price held constant.\n"
"\n"
"   Five time graphs are shown each for a different\n"
"stockprice and the user is shown two additional menus to\n"
"choose the price interval between the graphs and the\n"
"number of days over which the values are shown.\n"
"\n"
"   A vertical line on the graphs can be used to obtain\n"
"exact values and is moved with the cursor or tab keys.\n"
"\n"
"   The graph may be redrawn by pressing `R' or printed on\n"
"an compatible graphics printer using `P'.\n"
"                                          PRESS ANY KEY ";

static char *notice =
"   The rights for this program are reserved exclusively by\n"
"EFAM RESOURCES Pty. Ltd.\n"
"\n"
"   Users should carefully read the license agreement that\n"
"comes with this program.\n"
"\n"
"For updates or service contact:\n"
" Investment Software Systems Pty. Ltd.\n"
" PO. BOX  H37 Australia Square,\n"
" Sydney, N.S.W. 2000, Australia.\n"
" Telephone: (02) 27 4686  Fax: 27 5898\n"
"\n"
"\n"
"                                          PRESS ANY KEY ";



static char *helpindex[6]
= {{"Help Index"},
   {"Program Layout"},
   {"Entering values"},
   {"Function Keys"},
   {"Graphing"},
   {"Notice to Users"}};

static char *layout[5]
= {{"Program Layout"},
   {"General Layout"},
   {"Screen 1"},
   {"Screen 2"},
   {"Screen 3"}};

static char *entering[4]
= {{"Entering Values"},
   {"Movement"},
   {"Recalculation"},
   {"Editing"}};

static char *graphing[3]
= {{"Graphing"},
   {"Price Graphs"},
   {"Time Graphs"}};



void showhelp(char far *heading,char far *text)
{
makewindow(9,5,70,21,heading);
window(11,6,69,20);
gotoxy(1,2);
cprintf(text);
window(1,1,80,25);
waitkey();
closewindow(9,5,70,21);
}


int choosefunction()
{
char ch;
makewindow(11,7,68,13,"Function Keys");
window(13,8,67,12);
gotoxy(1,1);
cprintf("\nPress the function key that you require help with\n"
        "or any other key to return to the help menu");
window(1,1,80,25);
ch = getch();
closewindow(11,7,68,13);
if (ch != 0) return(0);
ch = getch();
return(ch);
}



void help()
{
int helpchoice;
int secondchoice;
int done = FALSE;
do
{
helpchoice = choice(5,helpindex);
switch (helpchoice)
{
case 0 : done = TRUE;break;
case 1 : secondchoice = choice(4,layout);
	 switch (secondchoice)
	 {
	 case 0 : break;
	 case 1 : showhelp("Help : General Layout",layout_1);break;
	 case 2 : showhelp("Help : Screen 1",layout_2);break;
	 case 3 : showhelp("Help : Screen 2",layout_3);break;
	 case 4 : showhelp("Help : Screen 3",layout_4);break;
	 }
	 break;
case 2 : secondchoice = choice(3,entering);
	 switch (secondchoice)
	 {
	 case 0 : break;
	 case 1 : showhelp("Help : Movement",entering_1);break;
	 case 2 : showhelp("Help : Recalculation",entering_2);break;
	 case 3 : showhelp("Help : Editing",entering_3);break;
	 }
	 break;
case 3 : secondchoice = choosefunction();
	 switch (secondchoice)
	 {
	 case 0 : break;
	 case F01 : showhelp("Help : F1",keys_1);break;
	 case F02 : showhelp("Help : F2",keys_2);break;
	 case F03 : showhelp("Help : F3",keys_3);break;
	 case F04 : showhelp("Help : F4",keys_4);break;
	 case F05 : showhelp("Help : F5",keys_5);break;
	 case F06 : showhelp("Help : F6",keys_6);break;
	 case F07 : showhelp("Help : F7",keys_7);break;
	 case F08 : showhelp("Help : F8",keys_8);break;
	 case F09 : showhelp("Help : F9",keys_9);break;
         case F10: showhelp("Help : F10",keys_10);break;
	 }
	 break;
case 4 : secondchoice = choice(2,graphing);
	 switch (secondchoice)
	 {
	 case 0 : break;
	 case 1 : showhelp("Help : Price Graphs",graphing_1);break;
	 case 2 : showhelp("Help : Time Graphs",graphing_2);break;
	 }
	 break;
case 5 : showhelp("Help : Notice to Users",notice); break;
}
} while (!done);
}


