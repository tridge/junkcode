

#include <graphics.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <conio.h>
#include <bios.h>
#include <ctype.h>
#include <stdlib.h>
#include "optdef.h"
#include "options.h"


double startfactor = 0.5;
int height,width[14], heightshift;
int xshift,xshiftinc;
int mapedready;
static char mapedfont[6][14][200];
char blank[200];



int gmode,gdriver;

double optpricedata[3][MAXPOINTS]; /* these for single option graphs */
double opttimedata[6][MAXPOINTS];
double optvoldata[3][MAXPOINTS];

char legend3[3][10];
char legend4[6][10];
char legend5[3][10];


static char *timechoice[6] =
{{"TimeGraph"},
 {"60 Days"},
 {"120 Days"},
 {"180 Days"},
 {"First expiry"},
 {"Last expiry"}};

static char *intervalchoice[5] =
{{"TimeGraph"},
 {"10 cents"},
 {"20 cents"},
 {"50 cents"},
 {"One dollar"}};



/* scaling constants */
/* first absolute... */
int screenminx,screenminy,screenmaxx,screenmaxy;
int maxpixx,maxpixy;

/* and world coords... */
double minx,maxx,miny,maxy;
double xgridsep,ygridsep;




int CRLF[2] = {13,10};
int STARTSEQ[13] = {27,51,24,13,10,27,65,8,27,42,0,23,03};
int LINSEQ[5] = {27,42,0,17,3};
int gtype; /* graph type*/
double gxval;
double yvals[5];
int rowdata[2][1000];
int graphrow= 0;
int oldrow;
int squashed = 0; /* 0 or 1 depending on where current data is stored */


/* then the relation between then */
double xscale,yscale;
/* so x = screenminx + (value-minx)*xscale */
/* shmultiplier returns the number of shares per contract */



void mapnum()
{
int i,j,k,gsetuptype;
char buf[2];
buf[0] = ' ';
buf[1] = 0;

if (!mapedready)
{
mapedready = TRUE;
for (i = 0; i <=9; i++)
{
buf[0] = '0' + i;
width[i] = textwidth(buf);
}
width[10] = textwidth("$");
width[11] = textwidth(".");
width[12] = textwidth(" ");
width[13] = textwidth("-");
height = textheight("8");
heightshift = 3.5 * height;
xshift =5;
xshiftinc= maxpixx/5;
getimage(10,10,10+width[8],10+height,blank);
if (sys.graphics.name[0] == 'H'|| sys.graphics.name[0] == 'M') gsetuptype = 1;
else if (sys.graphics.name[1] == 'C') gsetuptype = 2;
else gsetuptype = 5;
	 for (j=0; j < gsetuptype; j++)
{
 setcolor(sys.graphics.graphcolors[j+2]);
for (i =0; i<10; i++)
  {
  sprintf(buf,"%1d",i);
  outtextxy(10,10,buf);
  getimage(10,10,10+width[i],10+height,mapedfont[j][i]);
  putimage(10,10,blank,COPY_PUT);
  }

    outtextxy(10,10,"$");
  getimage(10,10,10+width[10],10+height,mapedfont[j][10]);
  putimage(10,10,blank,COPY_PUT);
    outtextxy(10,10,".");
  getimage(10,10,10+width[11],10+height,mapedfont[j][11]);
  putimage(10,10,blank,COPY_PUT);
    outtextxy(10,10," ");
  getimage(10,10,10+width[12],10+height,mapedfont[j][12]);
  putimage(10,10,blank,COPY_PUT);
    outtextxy(10,10,"-");
  getimage(10,10,10+width[13],10+height,mapedfont[j][13]);
  putimage(10,10,blank,COPY_PUT);
  }
    for (j = gsetuptype; j<5 ; j++)
  {
  for (i = 0; i <14; i++)
  for (k = 0; k< 200; k++)
  mapedfont[j][i][k] = mapedfont[gsetuptype -1][i][k];
  }

  setcolor(sys.graphics.graphcolors[1]);
  for (i =0; i<10; i++)
  {
  sprintf(buf,"%1d",i);
  outtextxy(10,10,buf);
  getimage(10,10,10+width[i],10+height,mapedfont[5][i]);
  putimage(10,10,blank,COPY_PUT);
  }
  outtextxy(10,10,"$");
  getimage(10,10,10+width[10],10+height,mapedfont[5][10]);
  putimage(10,10,blank,COPY_PUT);
    outtextxy(10,10,".");
  getimage(10,10,10+width[11],10+height,mapedfont[5][11]);
  putimage(10,10,blank,COPY_PUT);
    outtextxy(10,10," ");
  getimage(10,10,10+width[12],10+height,mapedfont[5][12]);
    putimage(10,10,blank,COPY_PUT);
    outtextxy(10,10,"-");
  getimage(10,10,10+width[13],10+height,mapedfont[5][13]);
  putimage(10,10,blank,COPY_PUT);
  }
}

void outnum(int x, int y,char *num,int colourt)
{
int i;
int swidth = 0;
i = 0;
for ( ; *num != 0;i++, num++)
     switch (*num)
     {
	  case ' ' : putimage(x + swidth,y, mapedfont[colourt][12],COPY_PUT);swidth+=width[12]; break;
	  case '.' : putimage(x + swidth,y, mapedfont[colourt][11],COPY_PUT);swidth+=width[11]; break;
	  case '$' : putimage(x + swidth,y, mapedfont[colourt][10],COPY_PUT);swidth+=width[10]; break;
	  case '-' : putimage(x + swidth,y, mapedfont[colourt][13],COPY_PUT);swidth+=width[13]; break;
	  default  : putimage(x + swidth,y, mapedfont[colourt][*num-'0'],COPY_PUT);swidth+=width[*num-'0']; break;
     }
 }

void chooselinestyle(int style)
{
switch (style)
{
	case 1 : setlinestyle(SOLID_LINE,0,NORM_WIDTH); break;
	case 2 : setlinestyle(DOTTED_LINE,0,NORM_WIDTH);break;
	case 3 : setlinestyle(USERBIT_LINE,3 ,NORM_WIDTH);break;
	case 4 : setlinestyle(CENTER_LINE,0,NORM_WIDTH);break;
}
}




void graphxval()
{
switch (gtype)
	{
     case 1 : gxval = status.pricedata[0][0] + (status.pricedata[0][numpts-1]
        - status.pricedata[0][0])*(graphrow- screenminx)/(screenmaxx - screenminx);
	      break; /* val graph */
    case 2 : gxval = status.timedata[0][0] + (status.timedata[0][numpts-1]
                                             - status.timedata[0][0]) *
	  (graphrow- screenminx)/(screenmaxx - screenminx);
		  break; /* time graph */
     case 3 : gxval = optpricedata[0][0] + (optpricedata[0][numpts-1]
                                             - optpricedata[0][0]) *
	  (graphrow- screenminx)/(screenmaxx - screenminx);
	      break; /* val graph */
    case 4 : gxval = opttimedata[0][0] + (opttimedata[0][numpts-1]
                                             - opttimedata[0][0]) *
	  (graphrow- screenminx)/(screenmaxx - screenminx);
		  break; /* time graph */
	}

}



void graphyval()
{
double xsepn;
double xplace;
int xposn;
switch (gtype)
	{
	case 1 : xplace = ((double)numpts-1)*(graphrow - screenminx)/(screenmaxx- screenminx);
		xposn = floor(xplace);
		xsepn = xplace - xposn;
		yvals[0] = (status.pricedata[1][xposn] * (1 - xsepn))
			     +(status.pricedata[1][xposn+1] *  xsepn);
		yvals[1] = (status.pricedata[2][xposn] * (1 - xsepn))
			     +(status.pricedata[2][xposn+1] *  xsepn);
                break; /* val graph */
	case 2 : xplace = ((double)numpts-1)*(graphrow - screenminx)/(screenmaxx- screenminx);
		xposn = floor(xplace);
		xsepn = xplace - xposn;
		yvals[0] = (status.timedata[1][xposn] * (1 - xsepn))
			     +(status.timedata[1][xposn+1] *  xsepn);
		yvals[1] = (status.timedata[2][xposn] * (1 - xsepn))
			     +(status.timedata[2][xposn+1] *  xsepn);
		yvals[2] = (status.timedata[3][xposn] * (1 - xsepn))
			     +(status.timedata[3][xposn+1] *  xsepn);
		yvals[3] = (status.timedata[4][xposn] * (1 - xsepn))
			     +(status.timedata[4][xposn+1] *  xsepn);
		yvals[4] = (status.timedata[5][xposn] * (1 - xsepn))
			     +(status.timedata[5][xposn+1] *  xsepn);
                         break; /* time graph */
	case 3 : xplace =((double)numpts-1)*(graphrow - screenminx)/(screenmaxx- screenminx);
		xposn = (int)floor(xplace);
		xsepn = xplace - xposn;
		yvals[0] = (optpricedata[1][xposn] * (1 - xsepn))
			     +(optpricedata[1][xposn+1] *  xsepn);
		yvals[1] = (optpricedata[2][xposn] * (1 - xsepn))
			     +(optpricedata[2][xposn+1] *  xsepn);
                break; /* val graph */
	case 4 : xplace = ((double)numpts-1)*(graphrow - screenminx)/(screenmaxx- screenminx);
		xposn = (int)floor(xplace);
		xsepn = xplace - xposn;
		yvals[0] = (opttimedata[1][xposn] * (1 - xsepn))
			     +(opttimedata[1][xposn+1] *  xsepn);
		yvals[1] = (opttimedata[2][xposn] * (1 - xsepn))
			     +(opttimedata[2][xposn+1] *  xsepn);
		yvals[2] = (opttimedata[3][xposn] * (1 - xsepn))
			     +(opttimedata[3][xposn+1] *  xsepn);
		yvals[3] = (opttimedata[4][xposn] * (1 - xsepn))
			     +(status.timedata[4][xposn+1] *  xsepn);
		yvals[4] = (opttimedata[5][xposn] * (1 - xsepn))
			     +(status.timedata[5][xposn+1] *  xsepn);
                         break; /* time graph */
	}
}


int blankimage[1000];


void starttext()
{
settextjustify(LEFT_TEXT,TOP_TEXT);
settextstyle(SMALL_FONT,HORIZ_DIR,sys.graphics.fontsizes[1]);
setcolor(sys.graphics.graphcolors[1]);
outtextxy(screenmaxx+15,screenmaxy-20,"X-Axis");
getimage(0,3.5*textheight("  "),maxpixx/5,5*textheight("  "),blankimage);
}



void graphvaluesout()
{
char values[5][12];
char xvalue[12];
int count;
int xval;
int charheight;
int numgrphs;
/*charheight = textheight("  ");*/
graphyval();
graphxval();
switch (gtype)
{
case 1 :
case 3 : numgrphs = 2;break;
case 2 :
case 4 : numgrphs = 5;break;
}
xval = xshift;
for (count=0;count<numgrphs;count++)
{
if (yvals[count] > 1e6 || yvals[count] < -1e6)
{
sprintf(values[count],"$%-9.*f",0,yvals[count]);
outnum(xval,heightshift,values[count],count);
}
else
{
sprintf(values[count],"$%-9.*f",2,yvals[count]);
outnum(xval,heightshift,values[count],count);
}
xval += xshiftinc;
}

switch (gtype)
{
case 1 :
case 3 : sprintf(xvalue,"$%-7.*f",2,gxval);
	 outnum(screenmaxx+15,screenmaxy-10,xvalue,5);
	 break;
case 2 :
case 4 : sprintf(xvalue,"%-3d ",(int) gxval);
	 outnum(screenmaxx+15,screenmaxy-10,xvalue,5);
	 break;
}
}







int graphmode()
{
detectgraph(&gdriver,&gmode);
if (gdriver < 0) return(gdriver);
if (gdriver == CGA && strncmp(sys.graphics.name,"CCGA",4) == 0)
gmode = CGAC0;
return(gdriver);
}



void startgraph()
{
initgraph(&gdriver,&gmode,"");
maxpixx = getmaxx();
maxpixy = getmaxy();
settextstyle(SMALL_FONT,HORIZ_DIR,sys.graphics.fontsizes[1]);
setbkcolor(sys.graphics.graphcolors[0]);
mapnum();
}



int gmaxx = 611;
int gmaxy = 319;
char    gstart[8] = "111";
char    gfin[8] = "222";
int     gtextlen = 5;
int     gstartlen = 4;
int	gbitlen;
int     gybitrows;
int     gfinlen = 4;
int     gtoplen = 8;
int     bitcode;
gvaltype gxvalout[10];
gvaltype gyvalout[10];
int xmark;
int ymark;
unsigned long gline2 = 65535;
unsigned long gline1 = 4294967295;
unsigned long gline3 = 16711935;
unsigned long gline4 = 252645135;
unsigned long gline5 = 62918655;
double gxscale,gyscale,gxgridsep,gygridsep;
double gmingridx,gmingridy,gminvalx,gminvaly;

ghline(int gy, unsigned long *lstyle, char data[][637])
{
int gcount1;
char *spot;
char bitsset;
spot = &data[gy/8][0];
bitsset = 1<<(gy%8);
for (gcount1 = 0; gcount1<= gmaxx; gcount1++)
{
if (1 == (*lstyle & 1))
 *spot++ |= bitsset;
 else
 spot++;
 *lstyle =_lrotl(*lstyle,1);
 }
}
gshline( unsigned long *lstyle, FILE *file)
{
int gcount1;
char spot;
char gstart1[6];
gstart1[0] = 27; gstart1[1] = 76; gstart1[2] = 100; gstart1[3] = 0;
for (gcount1 = 0; gcount1 <4; gcount1++)
fputc(gstart1[gcount1],file);
for (gcount1 = 0; gcount1< 100; gcount1++)
{
if (1 == (*lstyle & 1))
 fputc(8,file);
 else
 fputc(0,file);
 *lstyle =_lrotl(*lstyle,1);
 }
}
gvline(int gx, unsigned long *lstyle, char data[][637])
{
unsigned int gcount1;
for (gcount1 = 0; gcount1<= gmaxy; gcount1++)
{
if (*lstyle & 1)
 data[gcount1>>3][gx] |= 1<< (gcount1 & 7);
 *lstyle =_lrotl(*lstyle,1);
 }
}





gline(int gx1, int gy1, int gx2, int gy2, char data[][637],unsigned long  *lstyle)
{ unsigned  int i,y,x;
  unsigned int ydiff,xdiff;/* beware if drawing long lines with grad aprox = 1*/
  ydiff = gy2 - gy1;       /* As an overflow occures in the multiplication*/
  xdiff = gx2 - gx1;       /* of the unsigned ints before the devision */
  if (gy2 >= gy1)
  {
  ydiff = gy2 - gy1;
if (xdiff >= ydiff)
for (i = gx1; i <= gx2; i++)
 { if (1 == (*lstyle&1))
 { if (xdiff != 0) y = gy1 + ydiff * (i-gx1) / xdiff;
   else y = gy1;
  data[y>>3][i] |= 1 <<(y & 7);}
   *lstyle =_lrotl(*lstyle,1);

  }
  else
  for (i = gy1; i <= gy2; i++)
  {  if (1 == (*lstyle&1))
  { if (ydiff != 0)
  x = gx1 +xdiff * (i - gy1) / ydiff;
  else x = gx1;
  data[i>>3][x] |= 1 <<(i & 7);}
   *lstyle =_lrotl(*lstyle,1);
  }
  }
  else
    {
    ydiff =  gy1 - gy2;
if (xdiff >= ydiff)
for (i = gx1; i <= gx2; i++)
 {  if (1 == (*lstyle&1))
 {  if (xdiff != 0)
  y = gy1 - ydiff * (i-gx1) / xdiff;
  else y = gy1;
  data[y >> 3][i] |= 1 <<(y & 7);}
   *lstyle =_lrotl(*lstyle,1);
  }
  else
  for (i = gy2; i <= gy1; i++)
  {  if (1 == (*lstyle&1))
  {  if (ydiff != 0)
    x = gx2 - xdiff * ( i- gy2) / ydiff;
    else x = gx2;
  data[i>>3][x] |= 1 << (i & 7);}
   *lstyle =_lrotl(*lstyle,1);
  }
  }
}
gplotline(char data[][637])
{
int count1,count2;
int gx[2],gy[5][2],gx2,gy2[5];
int numgphs;
int first = 0;
int second = 1;

gline2 = 65535;
gline1 = 4294967295;
gline3 = 16711935;
gline4 = 252645135;
gline5 = 62918655;

  switch (gtype)
  {
   case 1 :    gx[first] = (status.pricedata[0][0]- minx)* gxscale;
	       gy[0][first] = (status.pricedata[1][0]- miny)* gyscale;
	       gy[1][first] = (status.pricedata[2][0]- miny)* gyscale;
	       for (count1 = 1; count1 < numpts;count1++)
		{ gx[second] = gxscale * (status.pricedata[0][count1]- minx);
	       gy[0][second] = (status.pricedata[1][count1]- miny)* gyscale;
	       gy[1][second] = (status.pricedata[2][count1]- miny)* gyscale;
	       gline(gx[first],gy[0][first],gx[second],gy[0][second],data,&gline1);
	       gline(gx[first],gy[1][first],gx[second],gy[1][second],data,&gline2);
	       first = first > 0 ?  0 : 1;
	       second = first > 0 ? 0 : 1;
		} break;
   case 2 :   gx[first] = (status.timedata[0][0]- minx)* gxscale;
	      gy[0][first] = (status.timedata[1][0]- miny)* gyscale;
	      gy[1][first] = (status.timedata[2][0]- miny)* gyscale;
	      gy[2][first] = (status.timedata[3][0]- miny)* gyscale;
	      gy[3][first] = (status.timedata[4][0]- miny)* gyscale;
	      gy[4][first] = (status.timedata[5][0]- miny)* gyscale;
	      for (count1 = 1; count1 < numpts;count1++)
		{ gx[second] = gxscale * (status.timedata[0][count1]- minx);
	       gy[0][second] = (status.timedata[1][count1]- miny)* gyscale;
	       gy[1][second] = (status.timedata[2][count1]- miny)* gyscale;
	       gy[2][second] = (status.timedata[3][count1]- miny)* gyscale;
	       gy[3][second] = (status.timedata[4][count1]- miny)* gyscale;
	       gy[4][second] = (status.timedata[5][count1]- miny)* gyscale;
	       gline(gx[first],gy[0][first],gx[second],gy[0][second],data,&gline1);
	       gline(gx[first],gy[1][first],gx[second],gy[1][second],data,&gline2);
               gline(gx[first],gy[2][first],gx[second],gy[2][second],data,&gline3);
	       gline(gx[first],gy[3][first],gx[second],gy[3][second],data,&gline4);
	       gline(gx[first],gy[4][first],gx[second],gy[4][second],data,&gline5);
	       first = first > 0 ?  0 : 1;
	       second = first > 0 ? 0 : 1;
		} break;
   case 3 :     gx[first] = (optpricedata[0][0]- minx)* gxscale;
	       gy[0][first] = (optpricedata[1][0]- miny)* gyscale;
	       gy[1][first] = (optpricedata[2][0]- miny)* gyscale;
	       for (count1 = 1; count1 < numpts;count1++)
		{ gx[second] = gxscale * (optpricedata[0][count1]- minx);
	       gy[0][second] = (optpricedata[1][count1]- miny)* gyscale;
	       gy[1][second] = (optpricedata[2][count1]- miny)* gyscale;
	       gline(gx[first],gy[0][first],gx[second],gy[0][second],data,&gline1);
	       gline(gx[first],gy[1][first],gx[second],gy[1][second],data,&gline2);
	       first = first > 0 ?  0 : 1;
	       second = first > 0 ? 0 : 1;
		} break;
   case 4 :    gx[first] = (opttimedata[0][0]- minx)* gxscale;
	       gy[0][first] = (opttimedata[1][0]- miny)* gyscale;
	       gy[1][first] = (opttimedata[2][0]- miny)* gyscale;
	       gy[2][first] = (opttimedata[3][0]- miny)* gyscale;
	       gy[3][first] = (opttimedata[4][0]- miny)* gyscale;
	       gy[4][first] = (opttimedata[5][0]- miny)* gyscale;
	       for (count1 = 1; count1 < numpts;count1++)
		{ gx[second] = gxscale * (opttimedata[0][count1]- minx);
	       gy[0][second] = (opttimedata[1][count1]- miny)* gyscale;
	       gy[1][second] = (opttimedata[2][count1]- miny)* gyscale;
	       gy[2][second] = (opttimedata[3][count1]- miny)* gyscale;
	       gy[3][second] = (opttimedata[4][count1]- miny)* gyscale;
	       gy[4][second] = (opttimedata[5][count1]- miny)* gyscale;
	       gline(gx[first],gy[0][first],gx[second],gy[0][second],data,&gline1);
	       gline(gx[first],gy[1][first],gx[second],gy[1][second],data,&gline2);
               gline(gx[first],gy[2][first],gx[second],gy[2][second],data,&gline3);
	       gline(gx[first],gy[3][first],gx[second],gy[3][second],data,&gline4);
	       gline(gx[first],gy[4][first],gx[second],gy[4][second],data,&gline5);
	       first = first > 0 ?  0 : 1;
	       second = first > 0 ? 0 : 1;
		} break;
   }
}
/*
gline(int gx1, int gy1, int gx2, int gy2, char data[][637],long int *lstyle)
moveto(screenminx + (data[0][0]-minx)*xscale,screenmaxy - (data[graphnum][0]-miny)*yscale);
for (i=1;i<numpts;i++)
{
delay(30);
lineto(screenminx + (data[0][i]-minx)*xscale,
       screenmaxy - (data[graphnum][i]-miny)*yscale);
} */

ggrid(char data[][637])
{
int i;
unsigned long dots1 =286331153;
unsigned long dots2 =33686018;
gmingridx =  (gminvalx-minx)* gxscale;
gmingridy =  (gminvaly-miny)* gyscale;
for (i=0; i<10; i++)
{ gxvalout[i].gposn = 0;
  gyvalout[i].gposn = 0;
  }

for (i= 0; gmingridx + i*xgridsep*gxscale < gmaxx; i++)
	{
	 gvline(gmingridx + i*xgridsep*gxscale,&dots1,data);
	 gxvalout[i].gval = gminvalx + i * xgridsep;
	 gxvalout[i].gposn = gmingridx + xgridsep * i* gxscale;
	 xmark = i;
	/* sprintf(numstr,"%-6.*f",xdec,minvalx + i*xgridsep);
	 while (numstr[strlen(numstr)-1] == ' ')
	 numstr[strlen(numstr)-1] = 0;
	 outtextxy(mingridx + i*xgridsep*xscale,screenmaxy+5,numstr);*/
	}
for (i= 0; gmingridy + i*ygridsep*gyscale < gmaxy; i++)
	{
	 ghline(gmingridy + i*ygridsep*gyscale ,&dots2,data);
	 gyvalout[i].gval = gminvaly + i * ygridsep;
	 gyvalout[i].gposn = gmingridy + ygridsep * i* gyscale;
	 ymark = i;
	}

}

gprintgraph()
{
int gcount,x,y,z;
int gcount1;
char filename1[8];
int gycount;
int gxcount;
int gxplace;
int gyplace;
double gnum;
int gyrow;
char gybit;

char gch;
double gyrowd,gpt1;

char *spot;
FILE *file;
char     graphspace[60][637];
char  lineout[9];
char ch1,ch2;
unsigned long lsolid = 4294967295;
char settab1[15];
int settabnum = 7;
settab1[0] = 27; settab1[1] = 68;settab1[2] = 11;settab1[3] = 22;
settab1[4] = 33; settab1[5] = 44; settab1[6] = 55; settab1[7] = 0;
settab1[8] =139; settab1[9] = 1; settab1[10] = 1; settab1[11] = 2;
settab1[12] = 0;

/*lsolid = ~lsolid;*/

gstart[0] = 27; gstart[1] = 76; gstart[2] = 100; gstart[3] = 2;
gfin[0] = 13; gfin[1] = 27; gfin[2] = 74; gfin[3] = 24;
strcpy(filename1, "PRN");
 if (((file = fopen(filename1, "wb")) == NULL) || (ferror(file) != 0))
 {
 errormessage("Printer Error\n\rCheck Hardware");
 return(0);
 }
 if (sys.prt == 1) {fputc(27,file); fputc(5,file); fputc(0,file);}/* IBM */
 fputc(13,file); fputc(10,file);
 strncpy(lineout,sys.date,7);
lineout[7]=0;
fprintf(file,"           DATE : %s     ", lineout);
if (ferror(file) != 0) return(0);
fprintf(file,"FILE : %s  \r\n ", filename);
for (gcount = 0; gcount < 49; gcount++)
{
spot = &graphspace[gcount][0];
for (gcount1 = 0; gcount1<= gmaxx; gcount1++)
 *spot++ = 0;
}
gvline(0,&lsolid,graphspace);
gvline(gmaxx,&lsolid,graphspace);
ghline(0,&lsolid,graphspace);
ghline(gmaxy,&lsolid,graphspace);
ggrid(graphspace);
gplotline(graphspace);
gline2 = 65535;
gline1 = 4294967295;
gline3 = 16711935;
gline4 = 252645135;
gline5 = 62918655;
for (y = 0; y < settabnum; y++)
fputc(settab1[y],file);
switch (gtype)
   {
    case 1 : fprintf(file," \t%s\t%s\r\n",status.legend1[0],status.legend1[1]);
	     fprintf(file," \t"); gshline(&gline1,file);
	     fprintf(file,"\t"); gshline(&gline2,file); break;
    case 2 : fprintf(file," \t%s\t%s\t%s\t%s\t%s\r\n",status.legend2[0],
	       status.legend2[1],status.legend2[2],status.legend2[3],
	       status.legend2[4]);
	       fprintf(file," \t"); gshline(&gline1,file);
	       fprintf(file,"\t"); gshline(&gline2,file);
	       fprintf(file,"\t"); gshline(&gline3,file);
	       fprintf(file,"\t"); gshline(&gline4,file);
	       fprintf(file,"\t"); gshline(&gline5,file);
	       break;
    case 3 : fprintf(file," \t%s\t%s\r\n",legend3[0],legend3[1]);
	      fprintf(file," \t"); gshline(&gline1,file);
	     fprintf(file,"\t"); gshline(&gline2,file);break;
    case 4 : fprintf(file,"  \t%s\t%s\t%s\t%s\t%s\r\n",legend4[0],
	       legend4[1],legend4[2],legend4[3],
	       legend4[4]);
	       fprintf(file," \t"); gshline(&gline1,file);
	       fprintf(file,"\t"); gshline(&gline2,file);
	       fprintf(file,"\t"); gshline(&gline3,file);
	       fprintf(file,"\t"); gshline(&gline4,file);
	       fprintf(file,"\t"); gshline(&gline5,file);
	       break;
   }
fprintf(file,"\r\n  VALUE ($)\r\n");
for (y= gmaxy/8; y >= 0; y--)
{
if ((gyvalout[ymark].gposn/8 == y) && (ymark >= 0))
{ gch = (3*gyvalout[ymark].gposn)%8;
fputc(13,file); fputc(27,file);fputc(74,file); fputc(24-gch,file);
fprintf(file,"%10.2f ", gyvalout[ymark].gval);
ymark--;
if (gch != 0)
{
fputc(27,file);fputc(74,file); fputc(gch,file);}
}
else { fputc(13,file);
    fputc(27,file);fputc(74,file); fputc(24,file);fprintf(file,"           ");
     }
for (z = 0; z < gstartlen; fputc(gstart[z++],file));
for (gcount = 0; gcount <= gmaxx; gcount++)
fputc(graphspace[y][gcount],file);
}
fprintf(file," \r\n");
for (y = 0; y <= xmark; y++)
{fprintf(file,"           "); fputc(27,file); fputc(76,file);
if (gxvalout[y].gposn > 10 && (gtype==2 || gtype == 4)) gxvalout[y].gposn-= 10;
else if ((gtype== 1||gtype ==3)&& gxvalout[y].gposn>30) gxvalout[y].gposn-=29;
ch1 = gxvalout[y].gposn%256; ch2 = gxvalout[y].gposn/256;
fputc(ch1,file);fputc(ch2,file);
for (z = 0; z < gxvalout[y].gposn; z++)
fputc(0,file);
if (gtype == 1 || gtype == 3)
fprintf(file,"$%-9.2f",gxvalout[y].gval);
else fprintf(file,"%d",(int)gxvalout[y].gval); fputc(13,file);

}
if (gtype == 1 || gtype == 3)
fprintf(file,"\r\n                                  PRICE  ($) \n");
else fprintf(file,"\r\n                                    DAYS  \n");
fputc(12,file);
fclose(file);
}



/*gmark(int gx1, int gy1,gprinttype *graphspace[])
{
int gyplace;
int gyrow;
char gybit;
double gyrowd;
   gyplace = 8* modf( gmaxy + gtoplen - gy1 ,&gyrowd);
   gyrow = gyrowd;
       switch (gyplace)
       {   case -1:
	   case 0 : gybit = 1; break;
	   case 1 : gybit = 2; break;
	   case 2 : gybit = 4; break;
	   case 3 : gybit = 8; break;
	   case 4 : gybit = 16; break;
	   case 5 : gybit = 32; break;
           case 6 : gybit = 64; break;
           case 7 : gybit = 128; break;
           case 8 : gybit = 128; break;
	   default : gybit = 0; break;
	}
(*graphspace[gyrow]).yvsx[gx1+ gstartlen+ gtextlen] &= gybit;


    }

glineto(int gx1, int gy1,int gx2,int gy2,gprinttype *graphspace[])
{
int gycount;
int gxcount;
int gxplace;
int gyplace;
double gnum;
int gyrow;
char gybit;

for (gxcount= 0;gxcount < (gx2- gx1); gxcount++)
   {
   if (gy2 - gy1 != 0)
   gnum = (gx2 - gx1)/(gy2 - gy1);
   else gnum = gx2 - gx1;
   if (gnum >= 0)
    for (gycount = 0; gycount <= gnum; gycount++)
    gmark(gx1 + gxcount, (gnum * gxcount) + gy1 +gycount,graphspace);
    else
    for (gycount = 0; gycount >= gnum; gycount--)
    gmark(gx1 + gxcount, (gnum * gxcount) + gy1 +gycount,graphspace);

  }
}
gboarder(gprinttype *graphspace[])
{
glineto(0,0,0,gmaxy,graphspace);
glineto(0,gmaxy,gmaxx,gmaxy,graphspace);
glineto(gmaxx,0,gmaxx,gmaxy,graphspace);
glineto(0,0,gmaxx,gmaxy,graphspace);
}   */

testit()
{
/*gboarder(graphspace);*/
gprintgraph();
beep();
}

void  border(int color)
{

moveto(screenminx,screenminy);
setcolor(color);
chooselinestyle(1);
lineto(screenmaxx,screenminy);
lineto(screenmaxx,screenmaxy);
lineto(screenminx,screenmaxy);
lineto(screenminx,screenminy);
}



void rowright()
{
oldrow = graphrow;
graphrow += 2;
if (graphrow > screenmaxx) graphrow = screenminx;
getimage(graphrow,screenminy, graphrow, screenmaxy, rowdata[!(squashed)]);
/* get next line*/
setcolor(sys.graphics.graphcolors[1]);
chooselinestyle(1);
line(graphrow,screenminy,graphrow,screenmaxy);
/* draw new line*/
putimage(oldrow,screenminy, rowdata[squashed],0); /* restore old line*/
squashed = !(squashed);
}



void rowleft()
{
oldrow = graphrow;
graphrow -= 2;
if (graphrow < screenminx) graphrow = screenmaxx;
getimage(graphrow,screenminy, graphrow, screenmaxy, rowdata[!(squashed)]);
/* get next line*/
setcolor(sys.graphics.graphcolors[1]);
chooselinestyle(1);
line(graphrow,screenminy,graphrow,screenmaxy);
/* draw new line*/
putimage(oldrow,screenminy, rowdata[squashed],0); /* restore old line*/
squashed = !(squashed);
}


void tabright()
{
int change;
change = (screenmaxx - screenminx)/8;
oldrow = graphrow;
graphrow += change;
if (graphrow > screenmaxx) graphrow = (graphrow-screenmaxx) + screenminx;
getimage(graphrow,screenminy, graphrow, screenmaxy, rowdata[!(squashed)]);
/* get next line*/
setcolor(sys.graphics.graphcolors[1]);
chooselinestyle(1);
line(graphrow,screenminy,graphrow,screenmaxy);
/* draw new line*/
putimage(oldrow,screenminy, rowdata[squashed],0); /* restore old line*/
squashed = !(squashed);
}

void tableft()
{
int change;
change = (screenmaxx - screenminx)/8;
oldrow = graphrow;
graphrow -= change;
if (graphrow < screenminx) graphrow = screenmaxx - (screenminx-graphrow);
getimage(graphrow,screenminy, graphrow, screenmaxy, rowdata[!(squashed)]);
/* get next line*/
setcolor(sys.graphics.graphcolors[1]);
chooselinestyle(1);
line(graphrow,screenminy,graphrow,screenmaxy);
/* draw new line*/
putimage(oldrow,screenminy, rowdata[squashed],0); /* restore old line*/
squashed = !(squashed);
}



double scalesep(double start,double finish)
{
static int maxticks = 5;
double diff,sep,power;
diff = finish-start;
if (diff < 0.00001) return(0.2);
power = floor(log10(diff));
sep = pow(10,power)*0.01;
do
{if (floor(diff/sep) > maxticks) sep *= 2.0;
 if (floor(diff/sep) > maxticks) sep *= 2.5;
 if (floor(diff/sep) > maxticks) sep *= 2.0;
} while (floor(diff/sep) > maxticks);
return(sep);
}






void title(int col1,int col2)
{
char titlestr[40];
char subtitle[40];
settextstyle(SMALL_FONT,HORIZ_DIR,sys.graphics.fontsizes[1]);
settextjustify(LEFT_TEXT,TOP_TEXT);
strcpy(titlestr,&startname[2]);
titlestr[28] = 0;
filename[13] = 0;
setcolor(col1);
outtextxy(5,1,titlestr);
strcpy(subtitle,"FILE : ");
strcat(subtitle, filename);
setcolor(col2);
settextjustify(RIGHT_TEXT,TOP_TEXT);
outtextxy(maxpixx-10,1,subtitle);
}


void findextrema(int numgraphs,double data[][MAXPOINTS])
{
int count,graphnum;
double ydiff;

screenminy = maxpixy/4;
screenmaxy = maxpixy - maxpixy/6;
screenminx = maxpixx/5;
screenmaxx = maxpixx - screenminx - 1;

minx = data[0][0];
maxx = minx;
miny = data[1][0];
maxy = miny;

for (count=0;count<numpts;count++)
{
if (data[0][count] < minx) minx = data[0][count];
else
if (data[0][count] > maxx) maxx = data[0][count];
}

for (graphnum=1;graphnum <= numgraphs;graphnum++)
for (count=0;count<numpts;count++)
{
if (data[graphnum][count] < miny) miny = data[graphnum][count];
else
if (data[graphnum][count] > maxy) maxy = data[graphnum][count];
}
if (minx == maxx) maxx = minx+1;
if (miny == maxy) maxy = miny +1;
else
{
ydiff = maxy - miny;
miny = miny - fabs(0.03 * ydiff);
maxy = maxy + fabs(0.03 * ydiff);
}
xscale = (screenmaxx-screenminx)/(maxx-minx);
yscale = (screenmaxy-screenminy)/(maxy-miny);
gxscale = (gmaxx)/(maxx-minx);
gyscale = (gmaxy)/(maxy-miny);
}




void grid(char xlegend[20],int xdec,int color)
{
int i;
char numstr[20];
double minvalx,minvaly;
int mingridx,maxgridy;

xgridsep = scalesep(minx,maxx);
ygridsep = scalesep(miny,maxy);
minvalx =gminvalx = xgridsep * ceil(minx/xgridsep);
minvaly =gminvaly = ygridsep * ceil(miny/ygridsep);
mingridx = screenminx + (minvalx-minx)*xscale;
maxgridy = screenmaxy - (minvaly-miny)*yscale;
if (maxx >= 100.0) xdec = 0;
setcolor(color);
chooselinestyle(2);
settextjustify(CENTER_TEXT,TOP_TEXT);
for (i= 0; mingridx + i*xgridsep*xscale < screenmaxx; i++)
	{
	 line(mingridx + i*xgridsep*xscale,screenmaxy,
	      mingridx + i*xgridsep*xscale,screenminy);
	 sprintf(numstr,"%-6.*f",xdec,minvalx + i*xgridsep);
	 while (numstr[strlen(numstr)-1] == ' ')
	 numstr[strlen(numstr)-1] = 0;
	 outtextxy(mingridx + i*xgridsep*xscale,screenmaxy+5,numstr);
	}
settextjustify(RIGHT_TEXT,CENTER_TEXT);
for (i= 0; maxgridy - i * ygridsep * yscale > screenminy;i++)
        {
	 line(screenminx, maxgridy - i * ygridsep * yscale,screenmaxx,maxgridy
	 - i * ygridsep * yscale);
	 if (fabs(minvaly+i*ygridsep) > 1.0e5)
	 sprintf(numstr,"%6.0f",minvaly + i * ygridsep);
	 else
	 sprintf(numstr,"%6.2f",minvaly + i * ygridsep);
	 outtextxy(screenminx-textheight(numstr),maxgridy - i * ygridsep*
	 yscale-1,numstr);
        }
strcpy(numstr,xlegend);
settextjustify(CENTER_TEXT,TOP_TEXT);
outtextxy((screenminx+screenmaxx)/2,maxpixy-textheight(numstr)-5,numstr);

strcpy(numstr,"Value ($)");
settextjustify(LEFT_TEXT,CENTER_TEXT);
settextstyle(SMALL_FONT,VERT_DIR,sys.graphics.fontsizes[1]);
outtextxy(screenmaxx+2*textheight(numstr),(screenmaxy+screenminy)/2,numstr);
settextstyle(SMALL_FONT,HORIZ_DIR,sys.graphics.fontsizes[1]);
}


void plot(int numgraphs,char legendnum[][10],double data[][MAXPOINTS])
{
int i,graphnum;
settextjustify(LEFT_TEXT,CENTER_TEXT);
settextstyle(SMALL_FONT,HORIZ_DIR,sys.graphics.fontsizes[1]);

for (graphnum=1;graphnum<=numgraphs;graphnum++)
{
chooselinestyle(sys.graphics.linestyles[graphnum-1]);
setcolor(sys.graphics.graphcolors[graphnum+1]);
legendnum[graphnum-1][9] = 0;


outtextxy(5+(graphnum-1)*maxpixx/5,2*textheight(legendnum[graphnum-1]),legendnum[graphnum-1]);
line(5+(graphnum-1)*maxpixx/5,3*textheight(legendnum[graphnum-1]),
     5+(graphnum-1)*maxpixx/5+textwidth(legendnum[graphnum-1]),
     3*textheight(legendnum[graphnum-1]));
if (graphnum == 1 && numgraphs == 5)
line(5+(graphnum-1)*maxpixx/5,3*textheight(legendnum[graphnum-1])-1,
     5+(graphnum-1)*maxpixx/5+textwidth(legendnum[graphnum-1]),
     3*textheight(legendnum[graphnum-1])-1);
if (gdriver == CGA && gmode == CGAHI)
delay(300);
moveto(screenminx + (data[0][0]-minx)*xscale,screenmaxy - (data[graphnum][0]-miny)*yscale);
for (i=1;i<numpts;i++)
{
if (gdriver == CGA && gmode == CGAHI)
delay(20);
lineto(screenminx + (data[0][i]-minx)*xscale,
       screenmaxy - (data[graphnum][i]-miny)*yscale);
}
if (graphnum == 1 && numgraphs == 5)
{
moveto(screenminx + (data[0][0]-minx)*xscale,screenmaxy-1 - (data[graphnum][0]-miny)*yscale);
for (i=1;i<numpts;i++)
{
lineto(screenminx + (data[0][i]-minx)*xscale,
       screenmaxy-1 - (data[graphnum][i]-miny)*yscale);
}
}
}
}


void anykey()
{

char outstr[20] = "Press Space to Exit";
char outstr1[20] = "P Print  R Redraw";
setcolor(sys.graphics.graphcolors[2]);
settextjustify(RIGHT_TEXT,TOP_TEXT);
settextstyle(SMALL_FONT,HORIZ_DIR,sys.graphics.fontsizes[1]);
outtextxy(maxpixx-5,maxpixy-1.5*textheight(outstr),outstr);
settextjustify(LEFT_TEXT,TOP_TEXT);
outtextxy(4,maxpixy-1.5*textheight(outstr1),outstr1);
}



void calcvalgraph()
{
double startp,endp,sepprice,totalval,tempval,dum,valuec,valuep;
double evaluec,evaluep,totaleval;
int count,row,edays,mdtx;

mdtx = mindtx();
startp = (1.0 -startfactor) *status.stockprice;
endp = (1.0 + startfactor)* status.stockprice;
sepprice = (endp-startp)/numpts;

/*for (count=0;count < 30;count++)
if (status.data[count].month >= 0 &&
   (status.data[count].heldc != 0 || status.data[count].heldp != 0))
if (status.sizepay[status.data[count].month].daysleft < edays)
    status.sizepay[status.data[count].month].daysleft = edays; */

for (count=0;count<numpts;count++)
{
totalval = (startp + (sepprice* count)) *  status.stockheld;
totaleval = (startp + (sepprice* count)) *  status.stockheld;
for (row=8;row<=37;row++)
if (status.data[row-8].month >= 0)
{
if (status.data[row-8].heldc != 0)
{
CBSdiv(status.volatility,
   status.data[row-8].strike,
   startp + count*sepprice,
   status.interest,
   dtx(status.data[row-8].month),
   &valuec,
   &dum);
totalval += shmultiplier(status.data[row-8].month) * valuec*status.data[row-8].heldc;
CBSdiv(status.volatility,
   status.data[row-8].strike,
   startp + count*sepprice,
   status.interest,
   dtx(status.data[row-8].month) - mdtx,
   &valuec,
   &dum);
totaleval += shmultiplier(status.data[row-8].month) * valuec*status.data[row-8].heldc;
}
if (status.data[row-8].heldp != 0)
{
PBSdiv(status.volatility,
   status.data[row-8].strike,
   startp + count*sepprice,
   status.interest,
   dtx(status.data[row-8].month),
   &valuep,
   &dum);
totalval += shmultiplier(status.data[row-8].month) * valuep * status.data[row-8].heldp;
PBSdiv(status.volatility,
   status.data[row-8].strike,
   startp + count*sepprice,
   status.interest,
   dtx(status.data[row-8].month) - mdtx,
   &valuep,
   &dum);
totaleval += shmultiplier(status.data[row-8].month) * valuep
             * status.data[row-8].heldp;
}
}

status.pricedata[0][count] = startp+count*sepprice;
status.pricedata[1][count] = totalval;
status.pricedata[2][count] = totaleval;
}
}




void calcvalgraph1()
{
double startp,endp,sepprice,totalval,tempval,dum,valuec,valuep;
double totaleval;
int count;

startp = (1.0 -startfactor) *status.stockprice;
endp = (1.0 + startfactor)* status.stockprice;
sepprice = (endp-startp)/numpts;

for (count=0;count<numpts;count++)
{
totalval = 0;
totaleval = 0;
if (status.data[sys.cell.row-8].month >= 0)
{
if (status.data[sys.cell.row-8].heldc != 0)
{
CBS(status.volatility,
   status.data[sys.cell.row-8].strike,
   startp + count*sepprice,
   status.interest,
   dtx(status.data[sys.cell.row-8].month),
   &valuec,
   &dum);
totalval += shmultiplier(status.data[sys.cell.row- 8].month)  *
	    valuec * status.data[sys.cell.row-8].heldc;
CBS(status.volatility,
   status.data[sys.cell.row-8].strike,
   startp + count*sepprice,
   status.interest,
   0,
   &valuec,
   &dum);
totaleval += shmultiplier(status.data[sys.cell.row- 8].month)  *
	    valuec * status.data[sys.cell.row-8].heldc;
}
if (status.data[sys.cell.row-8].heldp != 0)
{
PBS(status.volatility,
   status.data[sys.cell.row-8].strike,
   startp + count*sepprice,
   status.interest,
   dtx(status.data[sys.cell.row-8].month),
   &valuep,
   &dum);
totalval += shmultiplier(status.data[sys.cell.row- 8].month)  *
	    valuep * status.data[sys.cell.row-8].heldp;
PBS(status.volatility,
   status.data[sys.cell.row-8].strike,
   startp + count*sepprice,
   status.interest,
   0,
   &valuep,
   &dum);
totaleval += shmultiplier(status.data[sys.cell.row- 8].month)  *
	    valuep * status.data[sys.cell.row-8].heldp;
}
}

optpricedata[0][count] = startp+count*sepprice;
optpricedata[1][count] = totalval;
optpricedata[2][count] = totaleval;
}
}


void calctimegraph(int numdays,double interval)
{
double totalval,dum,valuec,valuep;
double evaluec,evaluep,totaleval;
int count,row,edays,startdays,graphnum,dayshift;
int fudge1[4];

for (count = 0;count < numpts;count++)
{
totalval = status.stockprice *  status.stockheld;
dayshift = floor(0.5 + count*numdays/(numpts -1));
initdivmtx(dayshift);
for (row=8;row<=37;row++)
if (status.data[row-8].month >= 0)
{
if (status.data[row-8].heldc != 0)
{
CBSdiv(status.volatility,
   status.data[row-8].strike,
   status.stockprice,
   status.interest,
   dtx(status.data[row-8].month) - dayshift,
   &valuec,
   &dum);

   totalval +=  shmultiplier(status.data[row-8].month) *
		valuec * status.data[row-8].heldc;
}
if (status.data[row-8].heldp != 0)
{
PBSdiv(status.volatility,
   status.data[row-8].strike,
   status.stockprice,
   status.interest,
   dtx(status.data[row-8].month) - dayshift,
   &valuep,
   &dum);


   totalval +=  shmultiplier(status.data[row-8].month) *
		valuep * status.data[row-8].heldp;
}
}
status.timedata[0][count] = dayshift;
status.timedata[1][count] = totalval;
}

 if (fabs(status.stockprice - (floor(status.stockprice/interval))*interval) < 0.01)
{ fudge1[0] = 1;
 fudge1[1] = 2;
 fudge1[2] = 4;
 fudge1[3] = 5;}
 else
{ fudge1[0] = 2;
 fudge1[1] = 3;
 fudge1[2] = 4;
 fudge1[3] = 5;}
for (graphnum = 2;graphnum <= 5;graphnum++)
for (count = 0;count < numpts;count++)
{
totalval = (floor(status.stockprice/interval)+fudge1[graphnum-2] -3)
           * interval * status.stockheld;
dayshift = floor(count*numdays/numpts +0.5);
initdivmtx(dayshift);
for (row=8;row<=37;row++)
if (status.data[row-8].month >= 0)
{
if (status.data[row-8].heldc != 0)
{
CBSdiv(status.volatility,
   status.data[row-8].strike,
   (floor(status.stockprice/interval)+fudge1[graphnum-2] -3)*interval,
   status.interest,
   dtx(status.data[row-8].month) - dayshift,
   &valuec,
   &dum);
   totalval += shmultiplier(status.data[row-8].month) *
	       valuec * status.data[row-8].heldc;
}
if (status.data[row-8].heldp != 0)
{
PBSdiv(status.volatility,
   status.data[row-8].strike,
   (floor(status.stockprice/interval)+fudge1[graphnum-2] -3)*interval,
   status.interest,
   dtx(status.data[row-8].month) - dayshift,
   &valuep,
   &dum);
   totalval += shmultiplier(status.data[row-8].month) *
	       valuep * status.data[row-8].heldp;
}
}

status.timedata[graphnum][count] = totalval;
}
sprintf(status.legend2[0],"$%-9.2f",status.stockprice);
for (graphnum = 2;graphnum <= 5;graphnum++)
sprintf(status.legend2[graphnum-1],"$%-9.2f",
	(floor(status.stockprice/interval)+fudge1[graphnum-2] -3)*interval);
initdivmtx(0);
}




void calctimegraph1(int numdays,double interval)
{
double totalval,dum,valuec,valuep;
double evaluec,evaluep,totaleval;
int count,edays,startdays,graphnum,dayshift;
int fudge1[4];

for (count = 0;count < numpts;count++)
{
totalval = status.stockprice *  status.stockheld;
dayshift = floor(0.5 + count*numdays/(numpts-1));
initdivmtx(dayshift);
if (status.data[sys.cell.row-8].month >= 0)
{
if (status.data[sys.cell.row-8].heldc != 0)
{
CBSdiv(status.volatility,
   status.data[sys.cell.row-8].strike,
   status.stockprice,
   status.interest,
   dtx(status.data[sys.cell.row-8].month) - dayshift,
   &valuec,
   &dum);

   totalval +=  shmultiplier(status.data[sys.cell.row-8].month) *
		valuec * status.data[sys.cell.row-8].heldc;
}
if (status.data[sys.cell.row-8].heldp != 0)
{
PBSdiv(status.volatility,
   status.data[sys.cell.row-8].strike,
   status.stockprice,
   status.interest,
   dtx(status.data[sys.cell.row-8].month) - dayshift,
   &valuep,
   &dum);

   totalval +=  shmultiplier(status.data[sys.cell.row-8].month) *
		valuep * status.data[sys.cell.row-8].heldp;
}
}

opttimedata[0][count] = dayshift;
opttimedata[1][count] = totalval;
}
 if (fabs(status.stockprice - (floor(status.stockprice/interval))*interval) < 0.01)
{ fudge1[0] = 1;
 fudge1[1] = 2;
 fudge1[2] = 4;
 fudge1[3] = 5;}
 else
{ fudge1[0] = 2;
 fudge1[1] = 3;
 fudge1[2] = 4;
 fudge1[3] = 5;}

for (graphnum = 2;graphnum <= 5;graphnum++)
for (count = 0;count < numpts;count++)
{
totalval = (floor(status.stockprice/interval)+fudge1[graphnum-2] -3) * interval
* status.stockheld;
dayshift = floor(count*numdays/numpts +0.5);
initdivmtx(dayshift);
if (status.data[sys.cell.row-8].month >= 0)
{
if (status.data[sys.cell.row-8].heldc != 0)
{
CBSdiv(status.volatility,
   status.data[sys.cell.row-8].strike,
   (floor(status.stockprice/interval)+fudge1[graphnum-2] -3)*interval,
   status.interest,
   dtx(status.data[sys.cell.row-8].month) - dayshift,
   &valuec,
   &dum);
   totalval += shmultiplier(status.data[sys.cell.row-8].month) *
	       valuec * status.data[sys.cell.row-8].heldc;
}
if (status.data[sys.cell.row-8].heldp != 0)
{
PBSdiv(status.volatility,
   status.data[sys.cell.row-8].strike,
   (floor(status.stockprice/interval)+fudge1[graphnum-2] -3)*interval,
   status.interest,
   dtx(status.data[sys.cell.row-8].month) - dayshift,
   &valuep,
   &dum);
   totalval += shmultiplier(status.data[sys.cell.row-8].month) *
	       valuep * status.data[sys.cell.row-8].heldp;
}
}
opttimedata[graphnum][count] = totalval;
}
sprintf(legend4[0],"$%-9.2f",status.stockprice);
for (graphnum = 2;graphnum <= 5;graphnum++)
sprintf(legend4[graphnum-1],"$%-9.2f",
	(floor(status.stockprice/interval)+fudge1[graphnum-2] -3)*interval);
initdivmtx(0);
}

int gettimechoice()
{
int ch;
ch = choice(5,timechoice);
if (ch < 4) return(ch*60);
if (ch == 4)
ch = mindtx();
else if (ch == 5)
ch = maxdtx();
if (ch < 15) ch = 15;
return(ch);
}

double getintervalchoice()
{
int ch;
ch = choice(4,intervalchoice);
switch (ch)
{ case 1 : return(0.1);
  case 2 : return(0.2);
  case 3 : return(0.5);
  case 4 : return(1.0);
  case 0 : return(0);
  default : return(0);
}
}

char *pricezoom[4] =
{{"Zoom Setting"},
{"Standard"},
{"Zoom In"},
{"Zoom Out"}
};

double getpricezoom()
{
int ch;
ch = choice(3,pricezoom);
switch (ch)
{ case 1: return(0.35);
  case 2: return(0.1);
  case 3: return(0.7);
  default: return(0.5);
  }
}




int drawgraph(int typeofgraph)
{
int done;
int redraw;
char ch;
int driver;
int timespan;
double interval;
int foundsome;
int row;
int moving = FALSE;
int printing = FALSE;
redraw = FALSE;
done = FALSE;
if (typeofgraph > 0 && typeofgraph <= 7)
{
driver = graphmode();
if (driver < 0) return(-1);

row = 0;
foundsome = FALSE;
while (!foundsome && row < 30)
{
if (status.data[row].month >= 0 &&
   (status.data[row].heldc != 0 || status.data[row].heldp != 0))
foundsome = TRUE;
row++;
}
if (status.stockheld != 0) foundsome = TRUE;

if (!foundsome && typeofgraph <= 4)
{
errormessage("No position held");
return(driver);
}

if ((typeofgraph == 3 || typeofgraph == 4) &&
                  (sys.cell.row < 8 || sys.screen != SCREEN1 ||
		  status.data[sys.cell.row-8].month < 0 ||
 		  (status.data[sys.cell.row-8].heldc == 0 &&
			  status.data[sys.cell.row-8].heldp == 0)))
{
errormessage("An active option\n\rmust be highlighted");
return(driver);
}
while (done == FALSE)
{
if (moving == FALSE)
{
if (printing == FALSE)
{
switch (typeofgraph)
{	case 1 : if (redraw == FALSE)
		 {startfactor = getpricezoom();
		 if (startfactor == 0.5) return(1);
		 wait();calcvalgraph();beep();
		 }
		 startgraph();
	 	 findextrema(2,status.pricedata);
 		 border(sys.graphics.graphcolors[1]);
		 grid("Price ($)",2,sys.graphics.graphcolors[1]);
		 title(sys.graphics.graphcolors[1],sys.graphics.graphcolors[2]);
		 sprintf(status.legend1[0],"Current   ");
		 sprintf(status.legend1[1],"Expiry    ");
		 plot(2,status.legend1,status.pricedata);
		 status.numpoints1 = numpts;
		 gtype = 1;
		 break;
	case 2 : if (redraw == FALSE)
		 { timespan = gettimechoice();
		 if (timespan <= 0) return(1);
		 interval = getintervalchoice();
		 if (interval <= 0.0) return(1);
		 wait();
		 calctimegraph(timespan,interval);
		 beep();
		 status.prevtimespan = timespan;
		 status.previnterval = interval;
		 }
		 startgraph();
	 	 findextrema(5,status.timedata);
 		 border(sys.graphics.graphcolors[1]);
		 grid("Days",0,sys.graphics.graphcolors[1]);
		 title(sys.graphics.graphcolors[1],sys.graphics.graphcolors[2]);
		 plot(5,status.legend2,status.timedata);
		 status.numpoints2 = numpts;
		 gtype = 2;
		 break;
	case 3 :
		 if (redraw == FALSE)
                 { startfactor = getpricezoom();
		 if (startfactor == 0.5) return(1);
		 wait();
		 calcvalgraph1();
		 beep();}
		 startgraph();
	 	 findextrema(2,optpricedata);
 		 border(sys.graphics.graphcolors[1]);
		 grid("Price ($)",2,sys.graphics.graphcolors[1]);
		 title(sys.graphics.graphcolors[1],sys.graphics.graphcolors[2]);
		 sprintf(legend3[0],"Current   ");
		 sprintf(legend3[1],"Expiry    ");
		 plot(2,legend3,optpricedata);
		 status.numpoints1 = numpts;
		 gtype = 3;
		 break;

	case 4 : if (redraw == FALSE)
		 { timespan = gettimechoice();
 		 if (timespan <= 0) return(1);
		 interval = getintervalchoice();
		 if (interval <= 0.0) return(1);
		 wait();
		 calctimegraph1(timespan,interval);
		 beep();       }
		 startgraph();
	 	 findextrema(5,opttimedata);
 		 border(sys.graphics.graphcolors[1]);
		 grid("Days",0,sys.graphics.graphcolors[1]);
		 title(sys.graphics.graphcolors[1],sys.graphics.graphcolors[2]);
		 plot(5,legend4,opttimedata);
		 status.numpoints2 = numpts;
		 gtype = 4;
		 break;
       case 5 :  numpts = status.numpoints1;
		  if (numpts <= 0 || numpts > MAXPOINTS) numpts = 15;
		 if (status.pricedata[0][numpts-1]-status.pricedata[0][0] <= 0.01)
		 {errormessage("No previous graph");return(1);}
		 startgraph();
		 findextrema(2,status.pricedata);
 		 border(sys.graphics.graphcolors[1]);
		 grid("Price ($)",2,sys.graphics.graphcolors[1]);
		 title(sys.graphics.graphcolors[1],sys.graphics.graphcolors[2]);
		 sprintf(status.legend1[0],"Current   ");
		 sprintf(status.legend1[1],"Expiry    ");
                 plot(2,status.legend1,status.pricedata);
		 gtype = 1;
		 break;
	case 6 : numpts = status.numpoints2;
		 if (numpts <= 0 || numpts > MAXPOINTS) numpts = 15;
		 if (status.timedata[0][numpts-1]-status.timedata[0][0] <= 1.0)
		 {errormessage("No previous graph");return(1);}
		 startgraph();
	 	 findextrema(5,status.timedata);
 		 border(sys.graphics.graphcolors[1]);
		 grid("Days",0,sys.graphics.graphcolors[1]);
		 title(sys.graphics.graphcolors[1],sys.graphics.graphcolors[2]);
		 plot(5,status.legend2,status.timedata);
		 gtype = 2;
		 break;
}
starttext();
graphrow = (screenmaxx - screenminx)/2 + screenminx;
getimage(graphrow,screenminy, graphrow, screenmaxy, rowdata[squashed]);
setcolor(sys.graphics.graphcolors[1]);
chooselinestyle(1);
line(graphrow,screenminy,graphrow,screenmaxy);
/* draw new line*/
anykey();
}
redraw = FALSE;
printing = FALSE;
moving = TRUE;
}
graphvaluesout();
ch = getch();
if (ch == 0)
{
ch = getch();
switch (ch)
  {
  case  ARROWR : rowright();
		 break;
  case ARROWL : rowleft();
		break;
  case LTAB :
		tableft();
		break;
  }
  }
else
switch (toupper(ch))
{
case 'P' :
/*	optpr();*/
	testit();
	printing = TRUE;
	moving = FALSE;
	break;
case 'R' :
	redraw = TRUE;
	moving = FALSE;
	break;
case TAB :
	tabright();
	break;
case ' ' : done = TRUE;
	   break;
default  : done = FALSE;
	   break;
}
} /* while */
}
closegraph();
textmode(2);
return(1);
}

