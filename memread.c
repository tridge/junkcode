#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/time.h>

struct timeval tp1,tp2;

static void start_timer()
{
  gettimeofday(&tp1,NULL);
}

static double end_timer()
{
  gettimeofday(&tp2,NULL);
  return((tp2.tv_sec - tp1.tv_sec) + 
	 (tp2.tv_usec - tp1.tv_usec)*1.0e-6);
}


#define MB (1024*1024)

static void fmemcpy(char *dest, char *src, int size)
{
        double d;
        double *dp, *sp;

        dp = (double *)dest;
        sp = (double *)src;
        
        while (size >= sizeof(d)) {
		d = sp[0];
		dp[0] = d;
                dp++; sp++;
                size -= sizeof(d);
        }
}

static void frame_copy(char *dst, char *src)
{
	int ysize = 512;
	while (ysize--) {
		dlmemcpy(dst, src, 512*2);
		src += 1024*2;
		dst += 1280*2;
	}

}

int main(int argc, char *argv[])
{
	int fd;
	char *p, *p2;
	int size = 4*MB;
	int i;

	fd = open(argv[1], O_RDWR);
	p = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0xe0000000);
	p2= mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0xe4400000 - 0x1000);
	close(fd);

	printf("p2=%p p=%p\n", p2, p);

	while (1)
		frame_copy(p, p2);

	return 0;
}
