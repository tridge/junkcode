#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <fnmatch.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

typedef unsigned BOOL;
#define True 1
#define False 0

#define TSERVER_PORT 8003
#define TSERVER_LOGFILE "tserver.log"

#define SAFE_FREE(v) ((v)?free(v):NULL, (v) = NULL)

#define MMAP_FAILED ((void *)-1)

#define INCLUDE_TAG "<!--#include virtual=\""
#define INCLUDE_TAG_END "\" -->"

/* prototypes */
void tcp_listener(int port, const char *logfile, void (*fn)(void));
void cgi_setup(void);
char *cgi_variable(char *name);
void dump_file(const char *fname);
void cgi_download(char *file);

void *map_file(const char *fname, size_t *size);
void unmap_file(const void *p, size_t size);



