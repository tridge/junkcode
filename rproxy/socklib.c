#include "rproxy.h"


/****************************************************************************
open a socket of the specified type, port and address for incoming data
****************************************************************************/
int open_socket_in(int type, int port, uint32 socket_addr)
{
	struct hostent *hp;
	struct sockaddr_in sock;
	char host_name[1000];
	int res;
	int one=1;

	/* get my host name */
	if (gethostname(host_name, sizeof(host_name)) == -1) { 
		fprintf(stderr,"gethostname failed\n"); return -1; 
	} 

	/* get host info */
	if ((hp = gethostbyname(host_name)) == 0) {
		fprintf(stderr,"Get_Hostbyname: Unknown host %s\n",host_name);
		return -1;
	}
  
	bzero((char *)&sock,sizeof(sock));
	memcpy((char *)&sock.sin_addr,(char *)hp->h_addr, hp->h_length);

#ifdef HAVE_SOCK_SIN_LEN
	sock.sin_len = sizeof(sock);
#endif
	sock.sin_port = htons( port );
	sock.sin_family = hp->h_addrtype;
	sock.sin_addr.s_addr = socket_addr;
	res = socket(hp->h_addrtype, type, 0);
	if (res == -1) { 
		fprintf(stderr, "socket failed\n"); return -1; 
	}

	setsockopt(res,SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof(one));

	/* now we've got a socket - we need to bind it */
	if (bind(res, (struct sockaddr * ) &sock,sizeof(sock)) < 0) { 
		return(-1); 
	}

	return res;
}


/* open a socket to a tcp remote host with the specified port 
   based on code from Warren */
int open_socket_out(char *host, int port)
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
		return -1;
	}

	memcpy(&sock_out.sin_addr, hp->h_addr, hp->h_length);
	sock_out.sin_port = htons(port);
	sock_out.sin_family = PF_INET;

	if (connect(res,(struct sockaddr *)&sock_out,sizeof(sock_out))) {
		close(res);
		fprintf(stderr,"failed to connect to %s - %s\n", 
			host, strerror(errno));
		return -1;
	}

	return res;
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
	  DEBUG(0,("Unknown socket option %s\n",tok));
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
	    DEBUG(0,("syntax error - %s does not take a value\n",tok));

	  {
	    int on = socket_options[i].value;
	    ret = setsockopt(fd,socket_options[i].level,
			     socket_options[i].option,(char *)&on,sizeof(int));
	  }
	  break;	  
	}
      
      if (ret != 0)
	DEBUG(0,("Failed to set socket option %s\n",tok));
    }
}


void start_accept_loop(int port, int (*fn)(int ))
{
	int s;

	/* open an incoming socket */
	s = open_socket_in(SOCK_STREAM, port, 0);
	if (s == -1) {
		perror("open_socket_in");
		exit(1);
	}

	/* ready to listen */
	if (listen(s, 5) == -1) {
		close(s);
		exit(1);
	}


	/* now accept incoming connections - forking a new process
	   for each incoming connection */
	while (1) {
		fd_set fds;
		int fd;
		struct sockaddr addr;
		int in_addrlen = sizeof(addr);

		FD_ZERO(&fds);
		FD_SET(s, &fds);

		if (select(s+1, &fds, NULL, NULL, NULL) != 1) {
			continue;
		}

		if(!FD_ISSET(s, &fds)) continue;

		fd = accept(s,&addr,&in_addrlen);

		if (fd == -1) continue;

		signal(SIGCHLD, SIG_IGN);

		/* we shouldn't have any children left hanging around
		   but I have had reports that on Digital Unix zombies
		   are produced, so this ensures that they are reaped */
#ifdef WNOHANG
		waitpid(-1, NULL, WNOHANG);
#endif

		if (fork()==0) {
			close(s);

			_exit(fn(fd));
		}

		close(fd);
	}
}
