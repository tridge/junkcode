/* a simple distributed lock manager server

   Andrew Tridgell (tridge@samba.org), February 2003

*/

#include "dl_server.h"

/*
  support functions for dl_server
*/



/*
  something really bad happened ....
*/
void fatal(const char *format, ...)
{
	va_list ap;
	char *ptr = NULL;

	va_start(ap, format);
	vasprintf(&ptr, format, ap);
	va_end(ap);
	
	if (!ptr || !*ptr) return;

	fprintf(stderr, "%s", ptr);

	free(ptr);
	exit(1);
}

/*
  keep writing until its all sent
*/
void write_all(int fd, const void *buf, size_t len)
{
	while (len) {
		int n = write(fd, buf, len);
		if (n <= 0) {
			fatal("failed to write packet\n");
		}
		buf = n + (char *)buf;
		len -= n;
	}
}

/*
  keep reading until its all read
*/
void read_all(int fd, void *buf, size_t len)
{
	while (len) {
		int n = read(fd, buf, len);
		if (n <= 0) {
			fatal("failed to read packet\n");
		}
		buf = n + (char *)buf;
		len -= n;
	}
}


static struct timeval tp1,tp2;

void start_timer(void)
{
	gettimeofday(&tp1,NULL);
}

double end_timer(void)
{
	gettimeofday(&tp2,NULL);
	return (tp2.tv_sec + (tp2.tv_usec*1.0e-6)) - 
		(tp1.tv_sec + (tp1.tv_usec*1.0e-6));
}

