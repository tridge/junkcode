
#include <string.h>
#include <conio.h>
#include "optdef.h"
#include "options.h"

/* help module for options analyst 27/5/88 */


static char *layout_1 =
"   The Options Analyst is divided into three screens. Each\n\r"
"of these screens may be accessed by selecting F6 and then\n\r"
"choosing appropriately from the menu.                   \n\r"
"                                                        \n\r"
"   SCREEN 1 provides theoretical valuation of individual\n\r"
"options and whole positions in a stock.                 \n\r"
"                                                        \n\r"
"   SCREEN 2 allows the user to enter data on the expiry \n\r"
"date of each month and specify any dividends that may be\n\r"
"given on the stock.                                     \n\r"
"                                                        \n\r"
"   SCREEN 3 provides volatility inversion and compares  \n\r"
"actual market prices to the theoretical values.         \n\r"
"                                          PRESS ANY KEY ";


static char *layout_2 =
"   The two middle columns of this screen show the       \n\r"
"theoretical values of the options in the left columns. \n\r"
"                                                        \n\r"
"   The two columns on the right of the screen may be used\n\r"
"either to show the theoretical delta of each corresponding\n\r"
"option or to enter a position in the market for valuation.\n\r"
"The F9 key is used to toggle between these uses.\n\r"
"                                                        \n\r"
"   The total call and put positions are shown at the    \n\r"
"bottom of the two right columns and the total theoretical\n\r"
"value/delta of the position is shown above these columns.\n\r"
"                                                        \n\r"
"   Two pages of options are available with PgUp/PgDown. \n\r"
"                                          PRESS ANY KEY ";


static char *layout_3 =
"   This screen is used to enter information about\n\r"
"dividends and expiry dates. The dividend day and amount in\n\r"
"cents is entered in the right two columns corresponding to\n\r"
"the month of the dividend.\n\r"
"\n\r"
"   The multiplier column is used for entering the\n\r"
"equivalent number of shares for each option contract.\n\r"
"\n\r"
"   The number of days between the current date and the\n\r"
"expiration date for each month is displayed in the Days-\n\r"
"Left column. Ensure the expiry day is correct for each\n\r"
"month.\n\r"
"                                          PRESS ANY KEY ";


static char *layout_4 =
"   This screen is used to calculate the implied volatility\n\r"
"corresponding to actual market prices. The market prices\n\r"
"are entered in the right two columns and the corresponding\n\r"
"implied volatilities are shown in the centre two columns.\n\r"
"\n\r"
"   The weighted put and call volatilities are shown at the\n\r"
"bottom of these two columns and the total weighted\n\r"
"volatility is shown in the top right corner.\n\r"
"\n\r"
"   By pressing F9 the display will toggle to showing the\n\r"
"percentage by which the theoretical values of Screen 1\n\r"
"differ from the market values. The display will still show\n\r"
"the weighted volatilities as before.\n\r"
"                                          PRESS ANY KEY ";

static char *entering_1 =
"   Movement around the screens is achieved by using the\n\r"
"four cursor keys normally located on the right hand side\n\r"
"of the keyboard. As you move into each cell it will be\n\r"
"highlighted.\n\r"
"\n\r"
"   The highlight bar will wrap around if you reach the top\n\r"
"of the screen and will start again from the bottom. \n\r"
"\n\r"
"   A second page of cells is available on screen 1 and\n\r"
"screen 3 using the PageUp and PageDown keys.\n\r"
"\n\r"
"   If you find that the cursor will not move correctly\n\r"
"check that the NumLock key is turned off.\n\r"
"                                          PRESS ANY KEY ";

static char *entering_2 =
"   The program will recalculate any values being currently\n\r"
"displayed as soon as any change has been made to the\n\r"
"values entered. The mathematical calculations required are\n\r"
"extremely complex and may take some time on an ordinary\n\r"
"PC.\n\r"
"\n\r"
"   If you find that the program is too slow at\n\r"
"recalculating values then we advise you to install a\n\r"
"mathematical co-processor in your machine. Speed increases\n\r"
"of up to twenty times have been noted with some machines\n\r"
"when a mathematical co-processor has been installed. The\n\r"
"program will automatically detect the presence of a co-\n\r"
"processor and use it for all mathematical operations.\n\r"
"                                          PRESS ANY KEY ";

static char *entering_3 =
"   To edit a cell first move the highlight bar into that\n\r"
"cell using the cursor keys. Values may then be entered\n\r"
"using the number keys at the top of the keyboard or using\n\r"
"the numeric keypad.\n\r"
"\n\r"
"   The date is entered in a similar fashion and should be\n\r"
"of the form DDMMMYY. (eg. 01JAN80)\n\r"
"\n\r"
"   Months and entered either by typing three letters (eg.\n\r"
"JAN) or by pressing the plus or minus keys to roll through\n\r"
"the months of the year. Pressing the plus key on an empty\n\r"
"month cell starts by entering the month of the cell above.\n\r"
"\n\r"
"                                          PRESS ANY KEY ";

static char *keys_1 =
"   This function key displays the help system that you are\n\r"
"currently using.\n\r"
"\n\r"
"   To escape from the help system and continue with the\n\r"
"program where you left off use the key marked <Esc>.\n\r"
"\n\r"
"   For more complete information on the program refer to\n\r"
"the users manual. If you still have any questions please\n\r"
"contact the distributor.\n\r"
"\n\r"
"\n\r"
"\n\r"
"\n\r"
"                                          PRESS ANY KEY ";

static char *keys_2 =
"   This command will save the current position in the\n\r"
"default directory after prompting you for a name for the\n\r"
"file.\n\r"
"\n\r"
"   The file name may be any normal DOS file name including\n\r"
"extension. Refer to your DOS manual for information on\n\r"
"legal file names.\n\r"
"\n\r"
"   The current file being used is displayed near the top\n\r"
"left corner of the screen.\n\r"
"\n\r"
"\n\r"
"\n\r"
"                                          PRESS ANY KEY ";

static char *keys_3 =
"   This comand will recall a file from the default\n\r"
"directory after prompting you for the name of the file. \n\r"
"\n\r"
"   If you cannot remember the names of the files you have\n\r"
"saved use the DIRECTORY command in the Utilities function.\n\r"
"\n\r"
"   The current file being used is displayed near the top\n\r"
"left corner of the screen.\n\r"
"\n\r"
"\n\r"
"\n\r"
"\n\r"
"\n\r"
"                                          PRESS ANY KEY ";

static char *keys_4 =
"   The F4 function key will sort the options currently\n\r"
"entered into the program in the traditional order for\n\r"
"options.\n\r"
"\n\r"
"   The options will be sorted first by month and secondly\n\r"
"by strike.\n\r"
"\n\r"
"   This allows additional strikes to be appended to the\n\r"
"end of the list and then sorted into their correct\n\r"
"position with one key.\n\r"
"\n\r"
"\n\r"
"\n\r"
"                                          PRESS ANY KEY ";

static char *keys_5 =
"   The F5 function key shows the graphing menu and allows\n\r"
"many combinations of option positions to be graphed.\n\r"
"\n\r"
"   A `Whole Position' graph will show a graph showing the\n\r"
"sum total of all the options and stock held in that stock.\n\r"
"The `This Option' graphs will show the position for the\n\r"
"call and put options held for the currently highlighted\n\r"
"row. An error message will appear if no position is held. \n\r"
"\n\r"
"   The previous `Whole Position' graphs are stored and can\n\r"
"be accessed using the `Previous' choices from the menu.\n\r"
"This saves recalculation of graphs which can take some\n\r"
"time due to the complex mathematics involved.\n\r"
"                                          PRESS ANY KEY ";

static char *keys_6 =
"   The F6 key shows the menu allowing the user to select\n\r"
"which of the three main screens is active.\n\r"
"\n\r"
"   For more information on the function of the three\n\r"
"screens see the program layout section in this help\n\r"
"system.\n\r"
"\n\r"
"\n\r"
"\n\r"
"\n\r"
"\n\r"
"\n\r"
"\n\r"
"                                          PRESS ANY KEY ";

static char *keys_7 =
"   The F7 key allows the user to quickly erase sections of\n\r"
"the entered data without individually blanking the cells.\n\r"
"\n\r"
"   The `Clear row' command will erase the row  that the\n\r"
"cursor is currently positioned at. Similarly the `Clear\n\r"
"Column' function will clear the column in which the cursor\n\r"
"is currently positioned. These functions will no work in\n\r"
"the Month or Multiplier columns or in the cells at the top\n\r"
"of the screens. The `Clear All' function will return the\n\r"
"program to the status it had when the program was called,\n\r"
"clearing all the entered data. \n\r"
"\n\r"
"   The data cannot be recovered once cleared.\n\r"
"                                          PRESS ANY KEY ";

static char *keys_8 =
"   The F8 key provides several useful utilities. The user\n\r"
"manual should be consulted to obtain more information on\n\r"
"the use of each of these functions.\n\r"
"\n\r"
"\n\r"
"\n\r"
"\n\r"
"\n\r"
"\n\r"
"\n\r"
"\n\r"
"\n\r"
"\n\r"
"                                          PRESS ANY KEY ";

static char *keys_9 =
"   The F9 key provides a toggle between the two types of\n\r"
"display for screen 1 and screen 3.\n\r"
"\n\r"
"   On screen 1 the display will switch between displaying\n\r"
"the currently held position and the theoretical Delta of\n\r"
"the position.\n\r"
"\n\r"
"   On screen 3 the display will toggle between the implied\n\r"
"volatilities, calculated from the market values entered by\n\r"
"the user, and the percentage by which the market values\n\r"
"are different from the theoretical values of screen 1.\n\r"
"\n\r"
"   The displays are toggled back by again pressing F9.\n\r"
"                                          PRESS ANY KEY ";

static char *keys_10 =
"   The F10 key exits the program and returns the user to\n\r"
"DOS.\n\r"
"\n\r"
"   The program will display a message asking the user if\n\r"
"he wishes to save before quitting. If you do not save to\n\r"
"disk then any changes you have made will be lost.\n\r"
"\n\r"
"\n\r"
"\n\r"
"\n\r"
"\n\r"
"\n\r"
"\n\r"
"                                          PRESS ANY KEY ";

static char *graphing_1 =
"   Price graphs show how the value of an option position\n\r"
"will change with changes in the underlying stock price.\n\r"
"\n\r"
"   The `Current' graph shows how the position would change\n\r"
"at the current time and the `Expiry' graph shows how the\n\r"
"position would change at the first expiry date of the\n\r"
"options currently held. \n\r"
"\n\r"
"   A vertical line on the graphs can be used to obtain\n\r"
"exact values and is moved with the cursor or tab keys.\n\r"
"The range of the Shareprice used depends on your choice\n\r"
"of Zoom factor. The graph may be redrawn by pressing 'R'\n\r"
"or printed (Compatible graphics printer) using `P'\n\r"
"                                          PRESS ANY KEY ";

static char *graphing_2 =
"   The time graphs shows the decay or growth of an option\n\r"
"position with time with the stock price held constant.\n\r"
"\n\r"
"   Five time graphs are shown each for a different\n\r"
"stockprice and the user is shown two additional menus to\n\r"
"choose the price interval between the graphs and the\n\r"
"number of days over which the values are shown.\n\r"
"\n\r"
"   A vertical line on the graphs can be used to obtain\n\r"
"exact values and is moved with the cursor or tab keys.\n\r"
"\n\r"
"   The graph may be redrawn by pressing `R' or printed on\n\r"
"an compatible graphics printer using `P'.\n\r"
"                                          PRESS ANY KEY ";

static char *notice =
"   The rights for this program are reserved exclusively by\n\r"
"EFAM RESOURCES Pty. Ltd.\n\r"
"\n\r"
"   Users should carefully read the license agreement that\n\r"
"comes with this program.\n\r"
"\n\r"
"For updates or service contact:\n\r"
" Investment Software Systems Pty. Ltd.\n\r"
" PO. BOX  H37 Australia Square,\n\r"
" Sydney, N.S.W. 2000, Australia.\n\r"
" Telephone: (02) 27 4686  Fax: 27 5898\n\r"
"\n\r"
"\n\r"
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



void showhelp(char *heading,char *text)
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
cprintf("\nPress the function key that you require help with\n\r"
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