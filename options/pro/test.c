#include <stdio.h>
#include <fcntl.h>
#include <stat.h>
#include <dos.h>
#include <bios.h>
#include <conio.h>

#include "optcolor.h"

#define BUFLEN 30000
#define NUMGRAPHS 5

int fhandle;
char buffer[BUFLEN];
char *string1;
long lastseek;
int j;
int adapter,row;


graphicstype *graph;
char *graphnames[] =
{{"CCGA"},
 {"MCGA"},
 {"HERC"},
 {"CEGA"},
 {"MEGA"}};

char *graphchoice[] =
{{"Adapter Types"},
 {"Color CGA"},
 {"Monochrome CGA"},
 {"Hercules"},
 {"Color EGA"},
 {"Monochrome EGA"}};

char *rowtypes[] =
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


char *find(char search_str[],char *buff,char *limit,int num)
{
int count = 0;
while (buff < limit)
{
if (*buff++ == search_str[count]) count++;
              else count = 0;
if (count == num) return(buff-num);
}
return(NULL);
}


void showgraph()
{
int i;
printf("\nNAME :         %4.4s\n",(*graph).name);
printf("COLORS :        ");
for (i=0;i<11;i++)
printf("%5d",(*graph).colors[i]);
printf("\nCURSOR :      ");
for (i=0;i<2;i++)
printf("%5d",(*graph).cursor[i]);
printf("\nGRAPHCOLORS : ");
for (i=0;i<8;i++)
printf("%5d",(*graph).graphcolors[i]);
printf("\nLINESTYLES :  ");
for (i=0;i<5;i++)
printf("%5d",(*graph).linestyles[i]);
printf("\nFONTSIZES :   ");
for (i=0;i<3;i++)
printf("%5d",(*graph).fontsizes[i]);
}


main()
{
clrscr();
adapter = 1;
if (adapter == 0) exit(0);
printf("Adapter : %s\n\n",graphchoice[adapter]);
for (row=0;row<21;row++)
printf("%s\n",rowtypes[row]);




fhandle = open("optdem.exe", O_RDWR | O_BINARY,
                              S_IREAD | S_IWRITE);


for (j=0;j<NUMGRAPHS;j++)
{
	do
	{
        lastseek = read(fhandle,&buffer,BUFLEN);
	graph = (graphicstype *) find(graphnames[j],buffer,&buffer[BUFLEN],4);
	} while (graph == NULL && !eof(fhandle));
if (graph == NULL)
	{
	printf("Un-Successful\n");
	lseek(fhandle,1,SEEK_SET);
	}
else
	{
	lseek(fhandle,-lastseek,SEEK_CUR);
	showgraph();
	}
}
close(fhandle);
}

