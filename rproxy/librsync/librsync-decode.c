#include "includes.h"

void
testrsyncusage(char * progname) {
  fprintf(stderr, "Usage: %s oldfile [dest [tokenstreamfile]]\n",progname);
  exit(1);
}

int
pread(void * readprivate, char * buf, int len, int offset) {
  int ret;

  if ((ret = lseek((int)readprivate,offset,SEEK_SET))==(off_t)-1) {
    return -errno;
  }
  return read ((int)readprivate,buf,len);
}

int
main(int argc, char *argv[]) {
  int totalwritten;
  int infd;
  int outfd;
  int littokfd;

  littokfd = 0;
  outfd = 1;

  switch(argc) {
  case(4):
    fprintf(stderr, "opening %s for reading\n", argv[3]);
    if ((littokfd = open(argv[3], O_RDONLY))<0) {
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


  totalwritten = librsync_decode((void *)infd,
				 (void *)outfd,
				 (void *)littokfd,
				 &pread,
				 &write,
				 &read
				 );
  close(infd);
  close(outfd);
  close(littokfd);
  if (totalwritten<0) {
    fprintf(stderr, "Error applying tokenstream: %s\n", strerror(-totalwritten));
    librsync_dump_logs();
    exit(EXIT_FAILURE);
  } 
  fprintf(stderr, "Total bytes written to outfile: %d\n", totalwritten);

  return 0;
}
