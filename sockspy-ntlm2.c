#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX(a,b) ((a)>(b)?(a):(b))

#define NTLMSSP_NEGOTIATE_UNICODE          0x00000001
#define NTLMSSP_NEGOTIATE_OEM              0x00000002
#define NTLMSSP_REQUEST_TARGET             0x00000004
#define NTLMSSP_NEGOTIATE_SIGN             0x00000010 /* Message integrity */
#define NTLMSSP_NEGOTIATE_SEAL             0x00000020 /* Message confidentiality */
#define NTLMSSP_NEGOTIATE_DATAGRAM_STYLE   0x00000040
#define NTLMSSP_NEGOTIATE_LM_KEY           0x00000080
#define NTLMSSP_NEGOTIATE_NETWARE          0x00000100
#define NTLMSSP_NEGOTIATE_NTLM             0x00000200
#define NTLMSSP_NEGOTIATE_DOMAIN_SUPPLIED  0x00001000
#define NTLMSSP_NEGOTIATE_WORKSTATION_SUPPLIED 0x00002000
#define NTLMSSP_NEGOTIATE_THIS_IS_LOCAL_CALL  0x00004000
#define NTLMSSP_NEGOTIATE_ALWAYS_SIGN      0x00008000
#define NTLMSSP_NEGOTIATE_128              0x20000000 /* 128-bit encryption */
#define NTLMSSP_NEGOTIATE_KEY_EXCH         0x40000000
#define NTLMSSP_NEGOTIATE_NTLM2            0x00080000

void dump_data(int level, const char *buf1,int len)
{
#define DEBUGADD(lvl, x) printf x
#define MIN(a,b) ((a)<(b)?(a):(b))
	void print_asc(int level, const uint8_t *buf,int len) {
		int i;
		for (i=0;i<len;i++)
			DEBUGADD(level,("%c", isprint(buf[i])?buf[i]:'.'));
	}
	const uint8_t *buf = (const uint8_t *)buf1;
	int i=0;
	if (len<=0) return;


	DEBUGADD(level,("[%03X] ",i));
	for (i=0;i<len;) {
		DEBUGADD(level,("%02X ",(int)buf[i]));
		i++;
		if (i%8 == 0) DEBUGADD(level,(" "));
		if (i%16 == 0) {      
			print_asc(level,&buf[i-16],8); DEBUGADD(level,(" "));
			print_asc(level,&buf[i-8],8); DEBUGADD(level,("\n"));
			if (i<len) DEBUGADD(level,("[%03X] ",i));
		}
	}
	if (i%16) {
		int n;
		n = 16 - (i%16);
		DEBUGADD(level,(" "));
		if (n>8) DEBUGADD(level,(" "));
		while (n--) DEBUGADD(level,("   "));
		n = MIN(8,i%16);
		print_asc(level,&buf[i-(i%16)],n); DEBUGADD(level,( " " ));
		n = (i%16) - n;
		if (n>0) print_asc(level,&buf[i-n],n); 
		DEBUGADD(level,("\n"));    
	}	
}

static void replace_str(char *buf, int n)
{
	static int count;
	printf("Packet %d\n", count++);
	dump_data(0, buf, n);
	if (0 && n == 0x40) {
		unsigned *x = (buf+0x10);
		printf("Changing 0x%x\n", *x);
		memcpy(x, "mnbv", 4);
	}
}

/* open a socket to a tcp remote host with the specified port */
static int open_socket_out(const char *host, int port)
{
	struct sockaddr_in sock_out;
	int res;
	struct hostent *hp;  
	struct in_addr addr;

	res = socket(PF_INET, SOCK_STREAM, 0);
	if (res == -1) {
		return -1;
	}

	if (inet_pton(AF_INET, host, &addr) > 0) {
		memcpy(&sock_out.sin_addr, &addr, sizeof(addr));
	} else {
		hp = gethostbyname(host);
		if (!hp) {
			fprintf(stderr,"unknown host %s\n", host);
			return -1;
		}
		memcpy(&sock_out.sin_addr, hp->h_addr, hp->h_length);
	}

	sock_out.sin_port = htons(port);
	sock_out.sin_family = PF_INET;

	if (connect(res,(struct sockaddr *)&sock_out,sizeof(sock_out)) != 0) {
		close(res);
		fprintf(stderr,"failed to connect to %s (%s)\n", 
			host, strerror(errno));
		return -1;
	}

	return res;
}


/*
  open a socket of the specified type, port and address for incoming data
*/
int open_socket_in(int port)
{
	struct sockaddr_in sock;
	int res;
	int one=1;

	memset(&sock,0,sizeof(sock));

#ifdef HAVE_SOCK_SIN_LEN
	sock.sin_len = sizeof(sock);
#endif
	sock.sin_port = htons(port);
	sock.sin_family = AF_INET;

	res = socket(AF_INET, SOCK_STREAM, 0);
	if (res == -1) { 
		fprintf(stderr, "socket failed\n"); return -1; 
		return -1;
	}

	setsockopt(res,SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof(one));

	if (bind(res, (struct sockaddr *)&sock, sizeof(sock)) < 0) { 
		return(-1); 
	}

	return res;
}

/* write to a file descriptor, making sure we get all the data out or
 * die trying */
static void write_all(int fd, unsigned char *s, size_t n)
{
	while (n) {
		int r;
		r = write(fd, s, n);
		if (r <= 0) {
			exit(1);
		}
		s += r;
		n -= r;
	}
}

static void main_loop(int sock1, int sock2)
{
	unsigned char buf[1024];
	int log1, log2, i=0;

	do {
		char *fname1, *fname2;
		asprintf(&fname1, "sockspy-in.%d", i);
		asprintf(&fname2, "sockspy-out.%d", i);
		log1 = open(fname1, O_WRONLY|O_CREAT|O_EXCL, 0644);
		log2 = open(fname2, O_WRONLY|O_CREAT|O_EXCL, 0644);
		free(fname1);
		free(fname2);
		i++;
	} while (i<1000 && (log1 == -1 || log2 == -1));

	if (log1 == -1 || log2 == -1) {
		fprintf(stderr,"Failed to open log files\n");
		return;
	}

	while (1) {
		fd_set fds;
		int ret;

		FD_ZERO(&fds);
		FD_SET(sock1, &fds);
		FD_SET(sock2, &fds);

		ret = select(MAX(sock1, sock2)+1, &fds, NULL, NULL, NULL);
		if (ret == -1 && errno == EINTR) continue;
		if (ret <= 0) break;

		if (FD_ISSET(sock1, &fds)) {
			int n = read(sock1, buf, sizeof(buf));
			if (n <= 0) break;

			replace_str(buf, n);

			write_all(sock2, buf, n);
			write_all(log1, buf, n);
		}

		if (FD_ISSET(sock2, &fds)) {
			int n = read(sock2, buf, sizeof(buf));
			if (n <= 0) break;

			replace_str(buf, n);

			write_all(sock1, buf, n);
			write_all(log2, buf, n);
		}
	}	
}

static char *get_socket_addr(int fd)
{
	struct sockaddr sa;
	struct sockaddr_in *sockin = (struct sockaddr_in *) (&sa);
	socklen_t length = sizeof(sa);
	static char addr_buf[200];

	strcpy(addr_buf,"0.0.0.0");

	if (fd == -1) {
		return addr_buf;
	}
	
	if (getsockname(fd, &sa, &length) < 0) {
		printf("getpeername failed. Error was %s\n", strerror(errno) );
		return addr_buf;
	}
	
	strcpy(addr_buf,(char *)inet_ntoa(sockin->sin_addr));
	
	return addr_buf;
}

int main(int argc, char *argv[])
{
	int listen_port, dest_port;
	char *host;
	int sock_in;
	int sock_out;
	int listen_fd;
	struct sockaddr addr;
	int in_addrlen = sizeof(addr);

	if (argc < 4) {
		printf("Usage: sockspy <inport> <host> <port>\n");
		exit(1);
	}

	listen_port = atoi(argv[1]);
	host = argv[2];
	dest_port = atoi(argv[3]);

	listen_fd = open_socket_in(listen_port);

	if (listen_fd == -1) {
		fprintf(stderr,"listen on port %d failed - %s\n", 
			listen_port, strerror(errno));
		exit(1);
	}

	if (listen(listen_fd, 5) == -1) {
		fprintf(stderr,"listen failed\n");
		exit(1);
	}

	sock_in = accept(listen_fd,&addr,&in_addrlen);

	if (sock_in == -1) {
		fprintf(stderr,"accept on port %d failed - %s\n", 
			listen_port, strerror(errno));
		exit(1);
	}

	printf("Connection from %s\n", get_socket_addr(sock_in));

	close(listen_fd);

	sock_out = open_socket_out(host, dest_port);
	if (sock_out == -1) {
		exit(1);
	}

	main_loop(sock_in, sock_out);
	return 0;
}
