/* Things to make charstar/fd interoperator easier */

#include "funkyfd.h"

#define LIBRSYNC_FUNKYFD_BUFMAGIC 0x1234FEDA
#define LIBRSYNC_FUNKYFD_SPACEINCREMENTS 16384

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif


#define check_magic(fd) \
if (fd->bufmagic != LIBRSYNC_FUNKYFD_BUFMAGIC) { \
  fprintf(stderr, "%s:%d passed non-buffer\n", __FILE__, __LINE__); \
  errno = EINVAL; \
  return -1; \
}


funkyfd_t *
librsync_funkyfd_new(char * buffer, int buflen) {
  funkyfd_t *tmp;
  int bufsize;

  if ((tmp = (funkyfd_t *)malloc(sizeof(funkyfd_t)))==NULL) {
    errno = ENOMEM;
    return NULL;
  }

  if (buffer && buflen>0) {
    bufsize = buflen;
  } else {
    bufsize = LIBRSYNC_FUNKYFD_SPACEINCREMENTS;
  }

  if ((tmp->buffer = malloc(bufsize))==NULL) {
    free(tmp);
    errno = ENOMEM;
    return NULL;
  }

  if (buffer) {
    memcpy(tmp->buffer,buffer,bufsize);
  }

  tmp->bufmagic = LIBRSYNC_FUNKYFD_BUFMAGIC;
  tmp->offset = 0;
  tmp->size = bufsize;
  tmp->buflen = buffer ? buflen : 0;

/*    memset(tmp->buffer, 'z', tmp->size); */

  return tmp;
}



int
librsync_funkyfd_destroy(funkyfd_t * fd) {

  check_magic(fd);

  if (fd->buffer) free (fd->buffer);
  free(fd);

  return 0;
}



int
librsync_funkyfd_pread(funkyfd_t *fd, char *buf, int len, int offset) {
  int copylen;

  check_magic(fd);

  copylen = MIN(len, fd->buflen-offset);

  memcpy(buf, fd->buffer+offset,copylen);

  return copylen;
}


int
librsync_funkyfd_read(funkyfd_t * fd, char * buf, int len) {
  int ret = 0;
  check_magic(fd);

  ret = librsync_funkyfd_pread(fd,buf,len,fd->offset);

  fd->offset += ret;
  return ret;
}


int
librsync_funkyfd_write(funkyfd_t * fd, char * buffer, int length) {
	check_magic(fd);
 
  if (length > fd->size-fd->buflen) {
/*      fprintf(stderr, "Gah! Out of buffer space. Lets realloc...\n"); */
    if ((fd->buffer =
	 realloc(fd->buffer, fd->size+LIBRSYNC_FUNKYFD_SPACEINCREMENTS))
	==NULL) {
	    errno = ENOMEM;
	    return -1;
    }
    fd->size +=LIBRSYNC_FUNKYFD_SPACEINCREMENTS;
  }
  memcpy(&(fd->buffer[fd->buflen]), buffer, length);
  fd->buflen+=length;
  
  return length;
}



int
librsync_funkyfd_to_charstar(funkyfd_t * fd,char ** fillme) 
{
  char * tmp;

  check_magic(fd);

  tmp = malloc(fd->buflen);
  if (!tmp) {
	  errno = ENOMEM;
	  return -1;
  }

  memcpy(tmp,fd->buffer,fd->buflen);

  *fillme = tmp;
  fprintf(stderr, "Setting fillme to %p\n", *fillme);

  return fd->buflen;
}
