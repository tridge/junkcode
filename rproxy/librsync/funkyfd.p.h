struct librsync_buf_with_space {
  int bufmagic;
  char *buffer;
  int offset;
  int size;
  int buflen;
};


typedef struct librsync_buf_with_space funkyfd_t;

