/* Things to make charstar/fd interoperator easier */

struct librsync_buf_with_space {
  int bufmagic;
  char *buffer;
  int spaceremaining;
  int offset;
  int size;
  int buflen;
};

funkyfd_t
librsync_funckyfd_open() {
}

int
librsync_funkyfd_close(funkyfd_t fd) {
}

char *
librsync_funkyfd_to_charstar()


int
librsync_funckyfd_read() {
}
