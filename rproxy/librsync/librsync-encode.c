#include "includes.h"

void
testrsyncusage(char * progname) {
  fprintf(stderr, "Usage: %s src [[dest [sigfile]]]\n",progname);
  exit(1);
}

int
main(int argc, char *argv[]) {
  int totalwritten;
  int infd;
  int outfd;
  int sigfd;

  sigfd = 0;
  outfd = 1;

  switch(argc) {
  case(4):
    fprintf(stderr, "opening %s for reading\n", argv[3]);
    if ((sigfd = open(argv[3], O_RDONLY))<0) {
      fprintf(stderr, "Unable to open %s for reading: %s\n", argv[3],
	      strerror(errno));
      exit(EXIT_FAILURE);
    }
    /* Drop through */
  case(3):
    fprintf(stderr, "opening %s for writing\n", argv[2]);
    if ((outfd = open(argv[2], O_WRONLY | O_CREAT,S_IWUSR|S_IRUSR))<0) {
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
    break;
  case(1):
  default:
    testrsyncusage(argv[0]);
  }

  totalwritten = librsync_encode((void *)infd,
				 (void *)outfd,
				 (void *)sigfd,
				 &read,
				 &write,
				 &read
				 );
  if (totalwritten<0) {
    fprintf(stderr, "Failed to make token stream: %s\n", strerror(-totalwritten));
    librsync_dump_logs();
    exit(EXIT_FAILURE);
  }
  librsync_dump_logs();
  fprintf(stderr, "Total bytes written to token stream: %d\n", totalwritten);

  close(infd);
  close(outfd);
  close(sigfd);

  return 0;
}
