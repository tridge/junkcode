#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>

#define FN
#define MAX_TRACKS 100

typedef char str[256];

int tty_fd=-1;
str device="/dev/gps";

char in_buff[1024];
int in_size=1024;
char *in_ptr=in_buff;
int in_cnt=0;


char out_buff[1024];
int out_size=1024;
char *out_ptr=in_buff;
int out_cnt=0;

extern FILE *dbf;

char *tracks[MAX_TRACKS];
int num_tracks=0;


FN void tty_puts(char *s);
FN void tty_close(void);


void dbg(char *s)
{
fprintf(dbf,"Debug: %s\n",s);
fflush(dbf);
}


void delay(int t)
{
usleep(10000*t);
}

int receive(char *fname)
{
  str command;
  fprintf(stderr,"%c",7);
  fflush(stderr);
  while (1);
  sprintf(command,"rx %s < %s > %s",fname,device,device);
  return(system(command));
}

void exittelix(int retcode, int hangup)
{
  if (hangup)
    {
      tty_close();
    }
  exit(retcode);
}

/*******************************************************************
add a track to the set of current tracks
********************************************************************/
FN int track(char *s,int tracknum)
{
  int i;
  
  for (i=0;i<num_tracks;i++)
    if (!tracks[i])
      {
	tracks[i] = (char *)malloc(strlen(s)+1);
	strcpy(tracks[i],s);
	dbg("reusing track");
	return(i);
      }
  
  if (num_tracks == MAX_TRACKS) 
    {
      dbg("no more tracks");
      return(-1);
    }
  tracks[num_tracks] = (char *)malloc(strlen(s)+1);
  strcpy(tracks[num_tracks],s);
  
  dbg("added track");
  
  num_tracks++;
  return(num_tracks - 1);
}

/*******************************************************************
check the list of tracks and return the index of a found track from 
"file"
********************************************************************/
FN int track_hit(int tracknum)
{
  char s[1024];
  int count=0;
  int i;
  int tries=30;

  memset(s,0,1024);

  while (tries)
    {
      int got = tty_gets(&s[count],1023-count); 
      count += got;
      if (got == 0) tries--;

      for (i=0;i<num_tracks;i++)
	if (tracks[i])
	  {
	    if ((strstr(s,tracks[i])!=NULL)) 
	      {
		fprintf(dbf,"Got: [%s]\n",s);
		fflush(dbf);
		return(i);
	      }
	  }
      if (count > 512)
	{
	  dbg("wrapping");
	  memcpy(s,s+(count-80),80);
	  count = 80;
	  memset(s+count,0,1024-count);
	}
    }
  return(-1);
}


/*******************************************************************
free a track
********************************************************************/
FN void track_free(int tracknum)
{
  fprintf(dbf,"freeing track %d\n",tracknum);
  if (tracks[tracknum]) free(tracks[tracknum]);
  tracks[tracknum] = NULL;
}




void set_terminal(char *term)
{
}

void set_port(int port)
{
  sprintf(device,"/dev/cua%d",port-1);
}

int tty_open(char *device)
{
  int i,speed;
  int fd;
  str command;
  struct termios tty;

  sprintf(command,"stty 0:1805:cbb:0:3:1c:7f:15:4:0:1:0:11:13:1a:0:12:f:17:16:0 < %s",device);
  system(command);
  
  fd = open(device,O_RDWR);
  
  ioctl(fd,TCGETS,&tty);
  tty.c_cflag &= ~CBAUD;
  tty.c_cflag |= B19200;
  ioctl(fd,TCSETS,&tty);

  ioctl(fd,FIONBIO);

  return(fd);
}


void terminal(void)
{
  dbg("terminal");
  if (tty_fd<0)
    {
      tty_fd = tty_open(device);
      if (tty_fd<0)
	{
	  fprintf(stderr,"Couldn't open port %s\n",device);
	  exittelix(1,1);	
	}
    }
}

void cputs(char *s)
{
  fprintf(dbf,"Sending: [%s]\n",s);
  fflush(dbf);
  tty_puts(s);
}


int timeout=0;

/*******************************************************************
signal this fn if something times out
********************************************************************/
FN void TimeOut(int sig)
{
  (void) sig;
  timeout = 1;
  dbg("timed out");
}


/*******************************************************************
get a string from the terminal
********************************************************************/
FN int tty_gets(char *s,int n)
{
  char c, c2, *p;
  int howlong=1;
  int count=0;
  void (*oldsig)(int);

  oldsig = signal(SIGALRM, TimeOut);
  (void) alarm(howlong);

  timeout = 0;
  while(!timeout && count<n) {
	c = (char) tty_getc();
	if (c == -1) continue;
	if (c==0) c=1;
	*s++ = c;
	count++;
  }
  (void) alarm(0);
  (void) signal(SIGALRM, oldsig);
  return(count);
}



/*******************************************************************
Read one character (byte) from the TTY link.
********************************************************************/
FN int tty_getc(void)
{
  int s;

  if (in_cnt == 0) {
    s = read(tty_fd, in_buff, in_size);
    if (s < 0) return(-1);
    in_cnt = s;
    in_ptr = in_buff;
  }

  if (in_cnt < 0) {
    dbg(" tty_getc: I/O error.\n");
    return(-1);
  }
  
  s = (int) *in_ptr;
  s &= 0xFF;
  in_ptr++;
  in_cnt--;
  putchar(s);
  return(s);
}


/*******************************************************************
Write one character (byte) to the TTY link.
********************************************************************/
FN int tty_putc(int c)
{
  int s;
  
  if ((out_cnt == out_size) || (c == -1)) {
    s = write(tty_fd, out_buff, out_cnt);
    out_cnt = 0;
    out_ptr = out_buff;
    if (s < 0) return(-1);
  }
  
  if (c != -1) {
    *out_ptr = (char) c;
    out_ptr++;
    out_cnt++;
  }
  
  return(0);
}



/*******************************************************************
Output a string of characters to the TTY link.
********************************************************************/
FN void tty_puts(char *s)
{
  while(*s != '\0') tty_putc((int) *s++);
  tty_putc(-1);	/* flush */
}

/*******************************************************************
close the tty
********************************************************************/
FN void tty_close(void)
{
  if (tty_fd>=0) close(tty_fd);
  tty_fd = -1;
}
