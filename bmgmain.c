#include <stdio.h>
#include <ctype.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>


static int hits=0;

void hit_fn(char *buf)
{
#if 0
  printf("hit[%d]=[%5.5s]\n",hits,buf);
#endif
  hits++;
}


/* a simple harness to test the above */
int main(int argc,char *argv[])
{
  char *target;
  char *buf;
  char *fname;
  int fd;
  int len;
  struct timeval t1,t2;

  if (argc < 3) {
    printf("Usage: bmg string|string|string... file\n");
    exit(1);
  }
   
  target = argv[1];
  fname = argv[2];
  fd = open(fname,O_RDONLY);
  if (fd < 0) {
    printf("couldn't open %s\n",fname);
    exit(1);
  }
  len = lseek(fd,0,SEEK_END);
  lseek(fd,0,SEEK_SET);
  buf = (char *)malloc(len);
  if (!buf) {
    printf("failed to alloc buffer\n");
    exit(1);
  }
  len = read(fd,buf,len);
  close(fd);
  printf("Loaded %d bytes\n",len);

  gettimeofday(&t1,NULL);
  bmg_build_table(target,NULL,0);
  gettimeofday(&t2,NULL);
  printf("Table build took %ld msecs\n",
	 (t2.tv_sec-t1.tv_sec)*1000 + (t2.tv_usec-t1.tv_usec)/1000);  

  gettimeofday(&t1,NULL);
  bmg_search(buf,len,hit_fn);
  gettimeofday(&t2,NULL);
  printf("Search took %ld msec\n",
	 (t2.tv_sec-t1.tv_sec)*1000 + (t2.tv_usec-t1.tv_usec)/1000);  
  printf("Got %d hits\n",hits);
  return(0);
}

