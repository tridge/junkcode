#include <fcntl.h>
#include <io.h>

char CRLF[2] = "\xa\xd";
char STARTSEQ[9] = "\x1b\x33\x18\x0d\x0a\x1b\x4c\x17\x03";
char LINSEQ[4] = "\x1b\x4c\x17\x03";

int xm;
int ym;
int backcolor;

int getpixl(float x, float y, float xf, float yf)
{
float xcurr;
int cx,cy;
int pv;
backcolor = 0;
pv = 0;
cy = (int) y;
while ( cy < (int) (y+ yf))
   {
  cx = (int) x;
   while ( cx < (int)(x + xf))
    {
    if (getpixel(x,y) != backcolor)
      {
       pv = 1;
       cy = (int) (y +yf);
       cx = (int) (x + xf);
       }
       }
       }
       return(pv);
}




optpr()
{
int xinc;
int yinc;
float xf,yf,c1x,c1y,c2y;
int count1, count2, count3;
int fhandle;

char ch;
int picrow;
fhandle = open("prn", O_RDWR | O_CREAT | O_TRUNC | O_BINARY,
                                   S_IWRITE);
xm= getmaxx();
ym= getmaxy();
xf = xm/785;
yf = ym/(44*9.0);
c1y = 0.0;
write(fhandle, STARTSEQ, 9);
for  (count1 = 1; count1 <= 44; count1++)
{
    c1x = 0.0;
    for (count2 = 0; count2 < (256*3 +17); count2++)
     {
       c2y = c1y;
       picrow =0;
       for (count3 = 1; count3 <= 8; count3++)
	 {
        picrow = (picrow * 2) +
			getpixl(c1x, c2y, c1x+ xf, c2y+yf);
	 c2y += yf;
	 }
	 ch = (char) picrow;
      write(fhandle, &ch,1);
	 c1x += xf;
      }
      write(fhandle, CRLF,2);
      if (count1 < 44)
      write(fhandle, LINSEQ,4);
      c1y += yf * 9.0;
}

	close(fhandle);
}
