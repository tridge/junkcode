#include "includes.h"
#include "funkyfd.h"


#define BLOCKLENGTH 128

#define SIGSPACE 16384
#define BUFMAGIC 0x1234FEDA

void showsums(void);

void do_base64_dump(char * astring, int len) {
  int i;
  fprintf(stderr, "In base64_dump\n");

  for (i=0;i<len;i++) {
    fprintf(stderr, "%d ", astring[i]);
  }
  fprintf(stderr, "\n");
}



ssize_t
pread(void * readprivate, char * buf, size_t len, off_t offset) {
  int ret;

  if ((ret = lseek((int)readprivate,offset,SEEK_SET))==(off_t)-1) {
    return -errno;
  }
  return read ((int)readprivate,buf,len);
}

void
testrsyncusage(char * progname) {
  fprintf(stderr, "Usage: %s old-copy-of-src src dest\n",progname);
  exit(1);
}

int
main(int argc, char *argv[]) {
  int totalwritten;
  int passwdfd;
  int infd;
  int outfd;
  funkyfd_t * funky_sigfd;
  funkyfd_t * funky_littokfd;
  char * mystring = NULL;
  int mystringlen;
/*    showsums(); */
  if (argc<4) testrsyncusage(argv[0]);

  funky_sigfd = librsync_funkyfd_new(NULL,0);
  if (funky_sigfd == NULL) {
    fprintf(stderr, "Failed to create funkyfd\n");
    exit(EXIT_FAILURE);
  }
  if (!(funky_littokfd = librsync_funkyfd_new(NULL,0))) {
    fprintf(stderr, "Failed to create funkyfd\n");
    exit(EXIT_FAILURE);
  }

  /* Start by generating the signatures to send to the sender. */
  if ((infd = open(argv[1], O_RDONLY))<0) {
    fprintf(stderr, "Unable to open %s for reading: %s\n", argv[1],
	    strerror(errno));
    exit(EXIT_FAILURE);
  }

  totalwritten = librsync_signature((void *)infd,
				    funky_sigfd,
				    &read,
				    &librsync_funkyfd_write,
				    BLOCKLENGTH);
  close (infd);

  mystringlen = librsync_funkyfd_to_charstar(funky_sigfd,&mystring);
  if (mystring) {
    fprintf(stderr, "Got signature string of length %d\n", mystringlen);
    do_base64_dump(mystring,mystringlen);
  }

  if (totalwritten<0) {
    fprintf(stderr, "Failed to make rsync signatures: %s\n", strerror(errno));
    librsync_dump_logs();
    exit(EXIT_FAILURE);
  }
  fprintf(stderr, "Total bytes written to signature: %d\n", totalwritten);

  

#ifdef DUMPLOGS
  librsync_dump_logs();
#endif

  /*The signature magically gets to the sender (which we are now acting as) */
  /* Generate the literal/token stream */

  if ((passwdfd = open(argv[2], O_RDONLY))<0) {
    fprintf(stderr, "Unable to open %s for reading: %s\n", argv[2],
	    strerror(errno));
    exit(EXIT_FAILURE);
  }
  totalwritten = librsync_encode((void *)passwdfd,
				 funky_littokfd,
				 funky_sigfd,
				 &read,
				 &librsync_funkyfd_write,
				 &librsync_funkyfd_read
				 );
  close(passwdfd);

  if (totalwritten<0) {
    fprintf(stderr, "Failed to make token stream: %s\n", strerror(errno));
    librsync_dump_logs();
    exit(EXIT_FAILURE);
  }
  fprintf(stderr, "Total bytes written to token stream: %d\n", totalwritten);

#ifdef DUMPLOGS
  librsync_dump_logs();
#endif

  /* Ok, we're now the receiver again. We have magically gotten this "stream"
     of tokens and literal data to read from...
  */

  if ((infd = open(argv[1], O_RDONLY))<0) {
    fprintf(stderr, "Unable to open %s for reading: %s\n",argv[1],
	    strerror(errno));
    exit(EXIT_FAILURE);
  }
  if ((outfd = open(argv[3], O_WRONLY|O_CREAT|O_TRUNC, S_IWUSR|S_IRUSR))<0) {
    fprintf(stderr, "Unable to open %s for writing: %s\n", argv[3], strerror(errno));  
    exit(EXIT_FAILURE);
  }

  totalwritten = librsync_decode((void *)infd,
				 (void *)outfd,
				 funky_littokfd,
				 &pread,
				 &write,
				 &librsync_funkyfd_read
				 );
  close(infd);
  close(outfd);

  librsync_funkyfd_destroy(funky_sigfd);
  librsync_funkyfd_destroy(funky_littokfd);

  if (totalwritten<0) {
    fprintf(stderr, "Error applying tokenstream: %s\n", strerror(errno));
    librsync_dump_logs();
    exit(EXIT_FAILURE);
  } 
  fprintf(stderr, "Total bytes written to outfile: %d\n", totalwritten);

#ifdef DUMPLOGS
  librsync_dump_logs();
#endif

  return 0;
}





void showsums(void) {
  int infd;
  int sum;
  int sum2;
  char buffer[2048];
  int i;
  int ret = 0;

  if ((infd = open("/etc/passwd", O_RDONLY))<0) {
    fprintf(stderr, "Unable to open passwd: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  if ((ret = read(infd,buffer,2048))<0) {
    fprintf(stderr, "Failed to read bytes: %s\n", strerror(errno));
    exit(1);
  };
  sum = get_checksum1(buffer,BLOCKLENGTH);
  fprintf(stderr, "First sum is %u\n", sum);
  fprintf(stderr, "Second sum is %u\n", get_checksum1(buffer+1,BLOCKLENGTH));
  fprintf(stderr, "Third sum is %u\n", get_checksum1(buffer+2,BLOCKLENGTH));
  for (i=0;i<ret; i++) {
    sum2 = get_checksum1(buffer+i+1,BLOCKLENGTH);
/*      fprintf(stderr, "initial sum: Next sum should be %d (%d %d)\n", */
/*  	    sum2,sum2>>16, */
/*  	    sum2&0xffff); */
    
/*      sum = next_sum(sum, buffer, 2048,i,BLOCKLENGTH); */
    fprintf(stderr, "i=%d: Got sum %u (%u %u)\n", i, sum, sum & 0xFFFF, sum >>16);
  }
  close (infd);
}
