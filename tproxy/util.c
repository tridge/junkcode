#include "includes.h"


/****************************************************************************
  create an outgoing socket. 
  **************************************************************************/
int open_socket_out(const char *dst, int port)
{
	struct sockaddr_in sock_out;
	int res;
	int one=1;
	struct in_addr addr;

	if (inet_pton(AF_INET, dst, &addr) <= 0) {
		struct hostent *hp;
		hp = gethostbyname(dst);
		if (!hp) return -1;
		memcpy(&addr, hp->h_addr, hp->h_length);
	}

	res = socket(PF_INET, SOCK_STREAM, 0);
	if (res == -1) {
		perror("socket");
		return -1;
	}

	memset((char *)&sock_out,'\0',sizeof(sock_out));
	sock_out.sin_addr = addr;
	sock_out.sin_port = htons(port);
	sock_out.sin_family = AF_INET;

	setsockopt(res,SOL_SOCKET,SO_BROADCAST,(char *)&one,sizeof(one));

	if (connect(res,(struct sockaddr *)&sock_out,sizeof(sock_out)) != 0) {
		perror("connect");
		return -1;
	}

	return res;
}


/****************************************************************************
listen on a tcp port then call fn() for each new connection with stdin and stdout
set to the socket and stderr pointing at logfile
****************************************************************************/
void tcp_listener(int port, void (*fn)(int ))
{
	struct sockaddr_in sock;
	int res;
	int one=1;
	int count = 0;

	memset((char *)&sock, 0, sizeof(sock));
	sock.sin_port = htons(port);
	sock.sin_family = AF_INET;
	res = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(res,SOL_SOCKET,SO_REUSEADDR,(char *)&one,sizeof(one));
	bind(res, (struct sockaddr * ) &sock,sizeof(sock));
	listen(res, 10);

	while (1) {
		int fd;

		fd = accept(res,NULL,0);
		if (fd == -1) continue;

		signal(SIGCHLD, SIG_IGN);

		if (fork() == 0) {
			close(res);
			/* setup stdin and stdout */
			fflush(stdout);
			fflush(stderr);
			dup2(fd, 0);
			dup2(fd, 1);
			close(fd);
			fn(count);
			fflush(stdout);
			_exit(0);
		}

		close(fd);
		count++;
	}
}


/*******************************************************************
mmap a file
********************************************************************/
void *map_file(const char *fname, size_t *size)
{
	struct stat st;
	void *p;
	int fd;

	fd = open(fname, O_RDONLY, 0);
	if (fd == -1) {
		fprintf(stderr, "Failed to load %s - %s\n", fname, strerror(errno));
		return NULL;
	}
	fstat(fd, &st);
	p = mmap(NULL, st.st_size+1, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FILE, fd, 0);
	close(fd);
	if (p == MMAP_FAILED) {
		fprintf(stderr, "Failed to mmap %s - %s\n", fname, strerror(errno));
		return NULL;
	}
	*size = st.st_size;

	/* make sure its terminated */
	*((char *)p+st.st_size) = 0;
	return p;
}

void unmap_file(void *p, size_t size)
{
	munmap(p, size+1);
}

void *x_malloc(size_t size)
{
	void *ret;
	ret = malloc(size);
	if (!ret) {
		fprintf(stderr, "Out of memory on size %d\n", (int)size);
		_exit(1);
	}
	return ret;
}


/* 
   trim the tail of a string
*/
void trim_tail(char *s, char *trim_chars)
{
	int len = strlen(s);
	while (len > 0 && strchr(trim_chars, s[len-1])) len--;
	s[len] = 0;
}


