#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <strings.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>


#define DL_PORT 7002


#define uint32 unsigned 
#define uint16 unsigned short
#define uint8 unsigned char

typedef uint32 NTSTATUS;

#define WRITE_LOCK 2
#define UN_LOCK 3

/* a locking request packet - in network byte order */
struct dl_request {
	/* offset and number of bytes to lock */
	uint32 offset;
	uint32 size;

	/* type of lock (WRITE_LOCK or UN_LOCK) */
	uint8 lock_type;
};

/* a locking reply packet - in network byte order */
struct dl_reply {
	uint32 status;
};


/* a handle for the client code */
struct dl_handle {
	int fd;
};


/* prototypes */
void process_packet(char *buf, size_t length, int sock_fd, struct sockaddr_in *from);

void fatal(const char *format, ...);
void write_all(int fd, const void *buf, size_t len);
void read_all(int fd, void *buf, size_t len);

uint32 lock_range(struct dl_handle *handle, int offset, int len);
uint32 unlock_range(struct dl_handle *handle, int offset, int len);
struct dl_handle *dlock_open(const char *host, int port);

void start_timer(void);
double end_timer(void);




/* some NT status codes - the brlock code was taken from Samba so it
 * has quite a few NTisms */
#define NT_STATUS_OK 0
#define NT_STATUS_RANGE_NOT_LOCKED (0x007e)
