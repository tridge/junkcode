#include <stdio.h>
#include <stdlib.h>

#define MAX_TRACKS 100
typedef char str[256];

FILE *dbf=NULL;

char **params;

int f,result;
str fname[14],wkday[7];
main(int argc,char *argv[])
{
   dbf = fopen("dbf","w");
   params = argv;
   set_terminal("VT102");
   set_port(2);
   terminal();
   cputs("\r\r");
   cputs("cl -q\r");
   filename(fname,wkday);
   download(result,wkday,fname);
   terminal();
   exittelix(0,1);
}

filename(str fname, str wkday)
{
  strcpy(fname,params[1]);
  strcpy(wkday,params[2]);
}

download(int result, str wkday, str fname)
{
int stat,t1,t2,t3,t4,t5,t6,t7,t8,t9;
t1 = track("Port? ",0);
t2 = track("Protocol(2)? ",0);
t3 = track("Data types to offload (oxcteahsrup; ~ = none)? ",0);
t4 = track("Terminate file transfer at end of stored data (y)? " ,0);
t5 = track("Style(0)? ",0);
t6 = track("Headers (h), trailers (t) and/or data introductions (i) [hi]? ",0);
t7 = track("([CR] to download all data)?",0);
t8 = track("Interval in days? ",0);
t9 = track("portlistner: TURN XMODEM ON NOW!",0);
stat = 0;
cputs("off\r");
while(1)
{
   terminal();
   stat = track_hit(0);
   if (stat == t1)
   {
      cputs("3\r");
      track_free(t1);
   }
   if (stat == t2)
   {
      cputs("2\r");
      track_free(t2);
   }
   if (stat == t3)
   {
      cputs("oxeahs\r");
      track_free(t3);
   }
   if (stat == t4)
   {
      cputs("y\r");
      track_free(t4);
   }
   if (stat == t5)
   {
      cputs("2\r");
      track_free(t5);
   }
   if (stat == t6)
   {
      cputs("\r");
      track_free(t6);
   }
   if (stat == t7)
   {
      cputs("g\r");
      track_free(t7);
   }
   if (stat == t8)
   {
      cputs(wkday);
      cputs("\r");
      track_free(t8);
      break;
   }
/*   if (stat == t9) */
/*      track_free(t9); */
}
delay(100);
result = receive(fname);
}

