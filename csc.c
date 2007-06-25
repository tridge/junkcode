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
   set_port(1);
   terminal();
   cputs("\r");
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
int stat,t0,t1,t2,t3,t4,t5,t6,t7=-2,t8,t9;

t0 = track("OK",0);
t1 = track("cscts:",0);
t2 = track("cscts:",0);
t3 = track("ogin:",0);
t4 = track("word:",0);
t5 = track("nimbus[",0);
t6 = track("nimbus[",0);
t8 = track("NO CARRIER",0);

stat = 0;
cputs("ath\r");
while(1)
{
   terminal();
   stat = track_hit(0);
   if (stat == t0)
   {
      cputs("atdt2495858\r");
      track_free(t0);
   }
   if (stat == t8)
   {
      cputs("atdt2495858\r");
   }
   if (stat == t1)
   {
      cputs("stty iflow eia oflow eia\r");
      track_free(t1);
   }
   if (stat == t2)
   {
      cputs("rlogin nimbus\r");
      track_free(t2);
   }
   if (stat == t3)
   {
      cputs("tridge\r");
      track_free(t3);
   }
   if (stat == t4)
   {
      cputs("fred\r");
      track_free(t4);
   }
   if (stat == t5)
   {
      cputs("ls\r");
      track_free(t5);
   }
   if (stat == t6)
   {
      cputs("logout\r");
      track_free(t6);
      t7 = track("cscts:");
   }
   if (stat == t7)
   {
      cputs("hang\r");
      track_free(t7);
   }
}

}
