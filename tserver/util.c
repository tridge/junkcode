#include "includes.h"

/****************************************************************************
listen on a tcp port then call fn() for each new connection with stdin and stdout
set to the socket and stderr pointing at logfile
****************************************************************************/
void tcp_listener(int port, const char *logfile, void (*fn)(void))
{
	struct sockaddr_in sock;
	int res;
	int one=1;

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
			close(0);
			close(1);
			dup(fd);
			dup(fd);
			close(fd);
			fn();
			fflush(stdout);
			_exit(0);
		}

		close(fd);
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
	p = mmap(NULL, st.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FILE, fd, 0);
	close(fd);
	if (p == MMAP_FAILED) {
		fprintf(stderr, "Failed to mmap %s - %s\n", fname, strerror(errno));
		return NULL;
	}
	*size = st.st_size;
	return p;
}

void unmap_file(const void *p, size_t size)
{
	munmap(p, size);
}
