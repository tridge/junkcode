#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>

// #include "snap8051.h"

#define SOCKET_NAME "/tmp/ux_demo"


/* some prototypes */
int ux_socket_connect(const char *name);
int ux_socket_listen(const char *name);
void *x_malloc(size_t size);
void *x_realloc(void *ptr, size_t size);
int fd_printf(int fd, const char *format, ...);
int send_packet(int fd, const char *buf, size_t len);
int recv_packet(int fd, char **buf, size_t *len);

