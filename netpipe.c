#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>

#include <signal.h>

#include <errno.h>


int FROM = 6005;
int TO = 6000;
char host_name[100];
struct hostent *hp;
struct sockaddr_in sock_in;
struct sockaddr_in sock_out;
int s;
struct sockaddr in_addr;
int in_addrlen;
int in_fd,out_fd;
#define BUFSIZE 4096
char buffer[BUFSIZE];

void transfer(int from,int to)
{
  int num_ready,num_read,ret;
  ioctl(from, FIONREAD, &num_ready);


  num_read = read(from,buffer,num_ready<BUFSIZE?num_ready:BUFSIZE);
  
  if (num_read == 0)
    {
      fprintf(stderr,"read 0 bytes. exiting.\n");
      exit(0);
    }

  ret = write(to,buffer,num_read);

  if (ret < 0)
	perror("(child) write");

  fprintf(stderr,"transfer(%d,%d): %d:%d\n",from,to,num_ready,ret);
}


int main(int argc,char *argv[])
{

  if (argc < 3)
	{
	  printf("Usage: netpipe <from_port> <to_port>\n");
	  return 0;
	}

  FROM = atoi(argv[1]);
  TO = atoi(argv[2]);
  
  /* get my host name */
  if (gethostname(host_name, sizeof(host_name)) == -1) 
    {
      perror("gethostname");
      return -1;
    } 

  /* get host info */
  if ((hp = gethostbyname(host_name)) == 0) 
    {
      /* Probly doesn't run TCP/IP. oh well. */
      fprintf(stderr, "Gethostbyname: Unknown host.\n");
      return -1;
    }

  memset(&sock_in, 0, sizeof(sock_in));
  memcpy(&sock_in.sin_addr, hp->h_addr, hp->h_length);
  sock_in.sin_port = htons( FROM );
  sock_in.sin_family = hp->h_addrtype;
  sock_in.sin_addr.s_addr = INADDR_ANY;
  s = socket(hp->h_addrtype, SOCK_STREAM, 0);
  if (s == -1) 
    {
      perror("socket");
      return -1;
    }

  /* now we've got a socket - we need to bind it */
  while (bind(s, (struct sockaddr * ) &sock_in,sizeof(sock_in)) < 0) 
    {
      perror("bind");
      close(s);
      return -1;
    }

  /* ready to listen */
  if (listen(s, 5) == -1) 
    {
      perror("listen");
      close(s);
      return -1;
    }


  /* now accept incoming connections - forking a new process
     for each incoming connection */
  fprintf(stderr,"waiting for a connection\n");
  while (in_fd = accept(s,&in_addr,&in_addrlen))
    {
      if (fork()==0)
	{
	  fprintf(stderr,"child is starting in_fd=%d\n",in_fd);
	  
	  /* create a socket to write to */
	  out_fd = socket(hp->h_addrtype, SOCK_STREAM, 0);
	  if (out_fd == -1) 
	    {
	      perror("(child) socket");
	      return -1;
	    }
	  
	  /* and connect it to the destination */
	  sock_out = sock_in;
	  sock_out.sin_port = htons(TO);
	  if (connect(out_fd,(struct sockaddr *)&sock_out,sizeof(sock_out))<0)
	    {
	      perror("(child) connect");
	      close(out_fd);
	      return -1;
	    }

	  /* now just do some reading and writing */
	  fprintf(stderr,"(child) entering select loop\n");
	  while (1)
	    {
	      fd_set fds;
	      int select_ret;

	      FD_ZERO(&fds);
	      FD_SET(in_fd, &fds);
	      FD_SET(out_fd, &fds);

	      if ((select_ret = select(255,&fds,NULL,NULL,NULL)) <= 0)
		{
		  perror("(child) select");
		  close(in_fd);
		  close(out_fd);
		  return -1;
		}

	      if (FD_ISSET(in_fd, &fds)) 
		transfer(in_fd,out_fd);

	      if (FD_ISSET(out_fd, &fds)) 
		transfer(out_fd,in_fd);
			 
	    }

	}
    }

  fprintf(stderr,"in_fd=%d. exiting.\n",in_fd);
  close(s);
}









