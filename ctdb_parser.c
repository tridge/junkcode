#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#define _STRING_LINE_(s)    #s
#define _STRING_LINE2_(s)   _STRING_LINE_(s)
#define __LINESTR__       _STRING_LINE2_(__LINE__)
#define __location__ __FILE__ ":" __LINESTR__

static void fatal(const char *msg)
{
	printf("%s\n", msg);
	exit(1);
}

int main(int argc, const char *argv[])
{
	int fd;
	unsigned len;
	unsigned ofs=0;

	fd = open(argv[1], O_RDONLY);
	
	while (read(fd, &len, sizeof(len)) == sizeof(len)) {
		unsigned char buf[len-4];
		ofs += 4;
		
		if (read(fd, buf, len-4) != (len-4)) {
			fatal(__location__);
		}
		ofs += len-4;
		printf("magic: %4.4s length=%u ofs=%u\n", buf, len, ofs);
	}
	
	return 0;
}
