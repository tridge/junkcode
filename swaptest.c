/* swaptest.c   0.1   (C) Jon Burgess 92jjb@eng.cam.ac.uk
 *
 * Tests the performance of the linux swap system
 * Generates a block of random data, reading it a number of times.
 */
/* TODO: options 
 *   <size>  memory size to try
 *   -time  do automatic timing
 *   -random  random pattern of data access
 *   -dirty dirty data on every access
 *   -cycles number of iterations of read to try.
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MEM_MB 64
#define MEM_KB (MEM_MB*1024)
#define MEM_SIZE (MEM_KB*1024)
#define ITERATIONS 2

main(int argc, char *argv[])
{
    char *buf;
    int i,ptr;
    char junk;
    unsigned long test;
    FILE *file;
    
    buf = (char *) malloc(MEM_SIZE);
    
#if 0
    file = fopen ("/dev/urandom","r");
    if (!file) {
      printf("Error reading /dev/random\n");
        exit(1);
    }

    /* Get a 1kB of random data */
    if (1 != fread( buf, 1024, 1, file)) {
         printf("Error reading random data\n");
         exit(1);
    }
    for (i=1; i<MEM_KB;i++)
       memcpy(buf+(i*1024),buf,1024);
#endif

    for(ptr = 0; ptr < MEM_SIZE; ptr+=4) {
      *(unsigned long *)(ptr + buf) = ptr;
    }
    
    for(i=0;i<ITERATIONS;i++) {
      printf("Running loop %d\n",i);
      for(ptr=0; ptr<MEM_SIZE; ptr+=4096){
#if 0
	junk =  *(ptr+buf);
#endif
	test = *(unsigned long *)(ptr + buf);
	if(test != ptr) {
	  printf("AIEEE: memory corrupted at %d, reads as %08lx\n",
		 ptr, test);
	}

	/* Do we dirty each page? */
#if 0
	*(ptr+buf) = 0;
#endif
      }
    }
    exit(0);
}

