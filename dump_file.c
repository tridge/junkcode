#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

typedef unsigned char uint8_t;
typedef unsigned uint32;

void dump_data(int level, const char *buf1,int len)
{
#define DEBUGADD(lvl, x) printf x
#define MIN(a,b) ((a)<(b)?(a):(b))
	void print_asc(int level, const uint8_t *buf,int len) {
		int i;
		for (i=0;i<len;i++)
			DEBUGADD(level,("%c", isprint(buf[i])?buf[i]:'.'));
	}
	const uint8_t *buf = (const uint8_t *)buf1;
	int i=0;
	if (len<=0) return;


	DEBUGADD(level,("[%06X] ",i));
	for (i=0;i<len;) {
		DEBUGADD(level,("%02X ",(int)buf[i]));
		i++;
		if (i%8 == 0) DEBUGADD(level,(" "));
		if (i%16 == 0) {      
			print_asc(level,&buf[i-16],8); DEBUGADD(level,(" "));
			print_asc(level,&buf[i-8],8); DEBUGADD(level,("\n"));
			if (i<len) DEBUGADD(level,("[%06X] ",i));
		}
	}
	if (i%16) {
		int n;
		n = 16 - (i%16);
		DEBUGADD(level,(" "));
		if (n>8) DEBUGADD(level,(" "));
		while (n--) DEBUGADD(level,("   "));
		n = MIN(8,i%16);
		print_asc(level,&buf[i-(i%16)],n); DEBUGADD(level,( " " ));
		n = (i%16) - n;
		if (n>0) print_asc(level,&buf[i-n],n); 
		DEBUGADD(level,("\n"));    
	}	
}

static void *map_file(char *fname, size_t *size)
{
	int fd = open(fname, O_RDONLY);
	struct stat st;
	void *p;
	
	fstat(fd, &st);
	p = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);

	*size = st.st_size;
	return p;
}

int main(int argc, char *argv[])
{
	unsigned char *p;
	size_t size;

	p = map_file(argv[1], &size);	

	dump_data(0, p, size);

	return 0;
}
