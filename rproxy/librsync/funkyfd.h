/* Things to make charstar/fd interoperator easier */

#include "funkyfd.p.h"

funkyfd_t *
librsync_funkyfd_new(char * buf, int length);

int
librsync_funkyfd_destroy(funkyfd_t * fd);

int
librsync_funkyfd_read(funkyfd_t * fd, char * buf, int len);

int
librsync_funkyfd_write(funkyfd_t * fd, char * buf, int len);


int
librsync_funkyfd_to_charstar(funkyfd_t * fd,char ** fillme);
