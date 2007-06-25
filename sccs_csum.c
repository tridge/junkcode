#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

/*
  calculate a simple checksum
*/
static unsigned sccs_checksum(const unsigned char *buf, size_t len)
{
	unsigned ret=0;
	int i;
	for (i=0;i<len;i++) {
		ret += buf[i];
	}
	return ret & 0xFFFF;
}

static void *map_file(const char *fname, size_t *size)
{
	int fd = open(fname, O_RDONLY);
	struct stat st;
	void *p;

	fstat(fd, &st);
	p = mmap(0, st.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	close(fd);

	*size = st.st_size;
	return p;
}


int main(int argc, const char *argv[])
{
	unsigned char *buf;
	size_t size;
	const char *fname = argv[1];
	unsigned sum;
	
	buf = map_file(fname, &size);

	sum = sccs_checksum(buf, size);

	printf("%05u\n", sum);
	return 0;
}
