#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORT 8999

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


static int open_socket_in(int type, int port, struct in_addr *address)
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
	if (address) {
		sock.sin_addr = *address;
	} else {
		sock.sin_addr.s_addr = INADDR_ANY;
	}
	res = socket(hp->h_addrtype, type, 0);
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

	return res;
}

void set_nonblocking(int fd)
{
	unsigned v = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, v | O_NONBLOCK);
}

static void child(int fd_in)
{
	int fd;
	struct sockaddr addr;
	int in_addrlen = sizeof(addr);

	if (listen(fd_in, 5) == -1) {
		close(fd_in);
		exit(1);
	}

	fd = accept(fd_in,&addr,&in_addrlen);
	
	sleep(20);
	exit(0);
}

static void parent(void)
{
	int fd = open_socket_out("localhost", PORT);
	fd_set fset;
	char c=0;
	int count=0;

	set_nonblocking(fd);

	while (1) {
		FD_ZERO(&fset);
		FD_SET(fd, &fset);
		if (select(8, NULL, &fset, NULL, NULL) == 1) {
			if (write(fd, &c, 1) == -1 && 
			    (errno == EAGAIN || errno == EWOULDBLOCK)) {
				printf("select returns when write would block\n");
				exit(1);
			}
			count++;
			if (count % 1024 == 0) {
				printf("%d\n", count);
			}
		}
	}
}


int main(void)
{
	int fd_in;

	fd_in = open_socket_in(SOCK_STREAM, PORT, NULL);

	fork() ? parent() : child(fd_in);

	return 0;
}
