#ifndef _LIBRSYNC_INCLUDES_H
#define _LIBRSYNC_INCLUDES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <strings.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "librsync.h"

#include "byteorder.h"

#define LIBRSYNC_VERSION 666

#define LIBRSYNC_MAXLOGLINELEN 80
#define LIBRSYNC_MAXLOGLINES 20

#define MD4_LENGTH 16
#define SUM_LENGTH 4
#define CHAR_OFFSET 0

#define MAXLITDATA 64*1024*1024


/* Hrrrmmm..... */
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

#include "mdfour.h"

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif


int librsync_get_version();
void dump_buffer (char *, int);

/* How about a "private" field for the receiver which gets echoed back to 
 * to the stream? This could be a strong sum of the file, or a file 
 * descriptor, or a hash of the filename (or simple lookup table), or...
 */

/* Stats stuff */
			    
struct librsync_stats_s {
  int falsealarms;
  int tag_hits;
  int matches;
};

int librsync_get_stats(struct librsync_stats_s foo);

struct sum_buf {
	off_t offset;		/* offset in file of this chunk */
	int len;		/* length of chunk of file */
	int i;			/* index of this chunk */
	uint32 sum1;	        /* simple checksum */
	char sum2[SUM_LENGTH];	/* checksum  */
};

struct sum_struct {
  off_t flength;		/* total file length */
  int count;			/* how many chunks */
  int remainder;		/* flength % block_length */
  int n;			/* block_length */
  struct sum_buf *sums;		/* points to info for each chunk */
};

struct map_struct {
	char *p;
	int fd,p_size,p_len;
	off_t file_size, p_offset, p_fd_offset;
};

uint32 get_checksum1(char *buf1,int len);
int get_checksum2(char *buf,int len,char *sum);
int build_hash_table(struct sum_struct *s);
int find_in_hash(int sum,char *(sum_fun)(void));



#endif
