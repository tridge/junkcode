#include "includes.h"

#define DEF_BLOCKLENGTH 256

void
testrsyncusage(char * progname) {
  fprintf(stderr, "Usage: %s oldfile [sigfile]\n",progname);
  exit(1);
}

int
main(int argc, char *argv[]) {
  int totalwritten;
  int infd;
  int outfd;
  int blocklen = DEF_BLOCKLENGTH;

  infd = 0;
  outfd = 1;

  switch(argc) {
  case(4):
    blocklen = atol(argv[3]);
    /* Drop through */
  case(3):
    fprintf(stderr, "opening %s for writing\n", argv[2]);
    if ((outfd = open(argv[2], O_WRONLY | O_CREAT))<0) {
      fprintf(stderr, "Unable to open %s for writing: %s\n", argv[2],
	      strerror(errno));
      exit(EXIT_FAILURE);
    }
    /* Drop through */
  case(2):
    fprintf(stderr, "opening %s for reading\n", argv[1]);
    if ((infd = open(argv[1], O_RDONLY))<0) {
      fprintf(stderr, "Unable to open %s for reading: %s\n", argv[1],
	      strerror(errno));
      exit(EXIT_FAILURE);
    }
  case(1):
    break;
  default:
    testrsyncusage(argv[0]);
  }

  totalwritten = librsync_signature((void *)infd,
				    (void *)outfd,
				    &read,
				    &write,
				    blocklen);
  if (totalwritten<0) {
    fprintf(stderr, "Failed to make signatures: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  close(outfd);
  close(infd);

  return 0;
}
