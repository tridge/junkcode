#include "config.h"

#include <sys/types.h>

#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#else
#include "lib/getopt.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <stddef.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#ifdef HAVE_SYS_FCNTL_H
#include <sys/fcntl.h>
#endif
#endif

#include <sys/stat.h>

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif

#include <signal.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#include <errno.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef HAVE_FNMATCH
#include <fnmatch.h>
#else
#include "lib/fnmatch.h"
#endif

#ifdef HAVE_GLOB_H
#include <glob.h>
#endif

#include <stdarg.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <syslog.h>
#include <sys/file.h>

#ifdef HAVE_COMPAT_H
#include <compat.h>
#endif

#define BOOL int

#ifndef uchar
#define uchar unsigned char
#endif

#if HAVE_UNSIGNED_CHAR
#define schar signed char
#else
#define schar char
#endif

#ifndef int32
#if (SIZEOF_INT == 4)
#define int32 int
#elif (SIZEOF_LONG == 4)
#define int32 long
#elif (SIZEOF_SHORT == 4)
#define int32 short
#else
/* I hope this works */
#define int32 int
#define LARGE_INT32
#endif
#endif

#ifndef uint32
#define uint32 unsigned int32
#endif

#include "dlinklist.h"
#include "librsync/rsync.h"
#include "librsync/sumset.h"

#define BUFFER_SIZE 512
#define CACHE_DIR "cache"
#define MIN_CACHE_SIZE 2048
#define MAX_SIG_SIZE 512

#define False 0
#define True 1

#define DEBUG(l, x) printf x

#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif

#include "mdfour.h"

#define PTR_DIFF(p1,p2) ((ptrdiff_t)(((const char *)(p1)) - (const char *)(p2)))

#include "proto.h"


void logmsg(const char *format, ...);
void errmsg(const char *format, ...);
