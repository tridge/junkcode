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
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>


#define uint32 unsigned 
#define BOOL int
#define False 0
#define True 1

#define DEBUG(l, x) printf x

int open_socket_in(int type, int port, uint32 socket_addr);
int open_socket_out(char *host, int port);
double end_timer();
void report_time(uint64_t total);
void start_timer();
void set_socket_options(int fd, char *options);





