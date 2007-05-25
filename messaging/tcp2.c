/* 
   measure latency of tcp sockets
   tridge@samba.org July 2006
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define TCP_NODELAY            1

typedef int BOOL;

#define True 1
#define False 0

static struct timeval tp1,tp2;

static void start_timer()
{
	gettimeofday(&tp1,NULL);
}

static double end_timer()
{
	gettimeofday(&tp2,NULL);
	return (tp2.tv_sec + (tp2.tv_usec*1.0e-6)) - 
		(tp1.tv_sec + (tp1.tv_usec*1.0e-6));
}

static void fatal(const char *why)
{
	fprintf(stderr, "fatal: %s - %s\n", why, strerror(errno));
	exit(1);
}

enum SOCK_OPT_TYPES {OPT_BOOL,OPT_INT,OPT_ON};

struct
{
  char *name;
  int level;
  int option;
  int value;
  int opttype;
} socket_options[] = {
  {"SO_KEEPALIVE",      SOL_SOCKET,    SO_KEEPALIVE,    0,                 OPT_BOOL},
  {"SO_REUSEADDR",      SOL_SOCKET,    SO_REUSEADDR,    0,                 OPT_BOOL},
  {"SO_BROADCAST",      SOL_SOCKET,    SO_BROADCAST,    0,                 OPT_BOOL},
#ifdef TCP_NODELAY
  {"TCP_NODELAY",       IPPROTO_TCP,   TCP_NODELAY,     0,                 OPT_BOOL},
#endif
#ifdef IPTOS_LOWDELAY
  {"IPTOS_LOWDELAY",    IPPROTO_IP,    IP_TOS,          IPTOS_LOWDELAY,    OPT_ON},
#endif
#ifdef IPTOS_THROUGHPUT
  {"IPTOS_THROUGHPUT",  IPPROTO_IP,    IP_TOS,          IPTOS_THROUGHPUT,  OPT_ON},
#endif
#ifdef SO_SNDBUF
  {"SO_SNDBUF",         SOL_SOCKET,    SO_SNDBUF,       0,                 OPT_INT},
#endif
#ifdef SO_RCVBUF
  {"SO_RCVBUF",         SOL_SOCKET,    SO_RCVBUF,       0,                 OPT_INT},
#endif
#ifdef SO_SNDLOWAT
  {"SO_SNDLOWAT",       SOL_SOCKET,    SO_SNDLOWAT,     0,                 OPT_INT},
#endif
#ifdef SO_RCVLOWAT
  {"SO_RCVLOWAT",       SOL_SOCKET,    SO_RCVLOWAT,     0,                 OPT_INT},
#endif
#ifdef SO_SNDTIMEO
  {"SO_SNDTIMEO",       SOL_SOCKET,    SO_SNDTIMEO,     0,                 OPT_INT},
#endif
#ifdef SO_RCVTIMEO
  {"SO_RCVTIMEO",       SOL_SOCKET,    SO_RCVTIMEO,     0,                 OPT_INT},
#endif
  {NULL,0,0,0,0}};


/****************************************************************************
  Get the next token from a string, return False if none found
  handles double-quotes. 
Based on a routine by GJC@VILLAGE.COM. 
Extensively modified by Andrew.Tridgell@anu.edu.au
****************************************************************************/
BOOL next_token(char **ptr,char *buff,char *sep)
{
  char *s;
  BOOL quoted;
  static char *last_ptr=NULL;

  if (!ptr) ptr = &last_ptr;
  if (!ptr) return(False);

  s = *ptr;

  /* default to simple separators */
  if (!sep) sep = " \t\n\r";

  /* find the first non sep char */
  while(*s && strchr(sep,*s)) s++;

  /* nothing left? */
  if (! *s) return(False);

  /* copy over the token */
  for (quoted = False; *s && (quoted || !strchr(sep,*s)); s++)
    {
      if (*s == '\"') 
	quoted = !quoted;
      else
	*buff++ = *s;
    }

  *ptr = (*s) ? s+1 : s;  
  *buff = 0;
  last_ptr = *ptr;

  return(True);
}
	

/****************************************************************************
set user socket options
****************************************************************************/
void set_socket_options(int fd, char *options)
{
	char tok[200];

	while (next_token(&options,tok," \t,"))
    {
      int ret=0,i;
      int value = 1;
      char *p;
      BOOL got_value = False;

      if ((p = strchr(tok,'=')))
	{
	  *p = 0;
	  value = atoi(p+1);
	  got_value = True;
	}

      for (i=0;socket_options[i].name;i++)
	if (strcasecmp(socket_options[i].name,tok)==0)
	  break;

      if (!socket_options[i].name)
	{
	  printf("Unknown socket option %s\n",tok);
	  continue;
	}

      switch (socket_options[i].opttype)
	{
	case OPT_BOOL:
	case OPT_INT:
	  ret = setsockopt(fd,socket_options[i].level,
			   socket_options[i].option,(char *)&value,sizeof(int));
	  break;

	case OPT_ON:
		if (got_value)
			printf("syntax error - %s does not take a value\n",tok);

	  {
	    int on = socket_options[i].value;
	    ret = setsockopt(fd,socket_options[i].level,
			     socket_options[i].option,(char *)&on,sizeof(int));
	  }
	  break;	  
	}
      
      if (ret != 0)
	printf("Failed to set socket option %s\n",tok);
    }
}

/*
  connect to a tcp socket
*/
int tcp_socket_connect(const char *host, int port)
{
	int type = SOCK_STREAM;
	struct sockaddr_in sock_out;
	int res;
	struct hostent *hp;

	res = socket(PF_INET, type, 0);
	if (res == -1) {
		return -1;
	}

	hp = gethostbyname(host);
	if (!hp) {
		fprintf(stderr,"unknown host: %s\n", host);
		close(res);
		return -1;
	}

	memcpy(&sock_out.sin_addr, hp->h_addr, hp->h_length);
	sock_out.sin_port = htons(port);
	sock_out.sin_family = PF_INET;

	if (connect(res,(struct sockaddr *)&sock_out,sizeof(sock_out))) {
		fprintf(stderr,"failed to connect to %s - %s\n", host, strerror(errno));
		close(res);
		return -1;
	}

	return res;
}


/*
  create a tcp socket and bind it
  return a file descriptor open on the socket 
*/
static int tcp_socket_bind(int port)
{
	struct hostent *hp;
	struct sockaddr_in sock;
	char host_name[1000];
	int res;
	int one=1;

	/* get my host name */
	if (gethostname(host_name, sizeof(host_name)) == -1) { 
		fprintf(stderr,"gethostname failed\n"); 
		return -1; 
	} 

	/* get host info */
	if ((hp = gethostbyname(host_name)) == 0) {
		fprintf(stderr,"gethostbyname: Unknown host %s\n",host_name);
		return -1;
	}
  
	memset((char *)&sock,0,sizeof(sock));
	memcpy((char *)&sock.sin_addr,(char *)hp->h_addr, hp->h_length);
	sock.sin_port = htons(port);
	sock.sin_family = hp->h_addrtype;
	sock.sin_addr.s_addr = INADDR_ANY;
	res = socket(hp->h_addrtype, SOCK_STREAM, 0);
	if (res == -1) { 
		fprintf(stderr,"socket failed\n"); 
		return -1; 
	}

	setsockopt(res,SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof(one));

	/* now we've got a socket - we need to bind it */
	if (bind(res, (struct sockaddr * ) &sock,sizeof(sock)) == -1) { 
		fprintf(stderr,"bind failed on port %d\n", port);
		close(res); 
		return -1;
	}

	listen(res, 1);

	return res;
}

/*
  accept on a tcp socket
*/
static int tcp_socket_accept(int fd)
{
	struct sockaddr a;
	socklen_t len;
	return accept(fd, &a, &len);
}

static void set_nonblocking(int fd)
{
	unsigned v = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, v | O_NONBLOCK);
}

static void worker(const char *host, int port1, int port2, int w)
{
	int l = tcp_socket_bind(port1);
	int s2, s1;
	int count=0;
	char c[4] = { 0, };
	int epoll_fd;
	struct epoll_event event;

	set_socket_options(l, "SO_REUSEADDR");

	sleep(2);

	s2 = tcp_socket_connect(host, port2);
	s1 = tcp_socket_accept(l);

	set_socket_options(s1, "TCP_NODELAY");
	set_socket_options(s2, "TCP_NODELAY");

	start_timer();

	epoll_fd = epoll_create(2);

	memset(&event, 0, sizeof(event));

	event.events = EPOLLIN | EPOLLERR | EPOLLHUP;
	event.data.fd = s1;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, s1, &event);

	set_nonblocking(s1);

	while (1) {
		struct epoll_event events[8];
		int ret;
		int num_ready;

		*(int*)c = count;
		
		if (write(s2, c, sizeof(c)) != sizeof(c)) {
			fatal("write");
		}
		ret = epoll_wait(epoll_fd, events, 8, -1);
		if (ret != 1) fatal("epoll_wait");

		if (ioctl(s1, FIONREAD, &num_ready) != 0) fatal("FIONREAD");

		printf("num_ready=%d\n", num_ready);

//		if (num_ready != sizeof(c)) fatal("num_ready");
	     
		if (read(s1, c, sizeof(c)) != sizeof(c)) {
			fatal("read");
		}
		if (w == 1 && (end_timer() > 1.0)) {
			printf("%8u ops/sec\r", 
			       (unsigned)(2*count/end_timer()));
			fflush(stdout);
			start_timer();
			count=0;
		}
		count++;
	}
}

int main(int argc, char *argv[])
{
	worker(argv[1], atoi(argv[2]), atoi(argv[3]), 1);
	return 0;
}
