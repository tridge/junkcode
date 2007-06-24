/*
  look for raw deflate regions in a file
 */

#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define DEBUGADD(l, x) printf x

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

static void print_asc(int level, const unsigned char *buf,int len)
{
	int i;
	for (i=0;i<len;i++)
		DEBUGADD(level,("%c", isprint(buf[i])?buf[i]:'.'));
}

static void dump_data(int level, const unsigned char *buf,int len)
{
	int i=0;
	if (len<=0) return;

	DEBUGADD(level,("[%03X] ",i));
	for (i=0;i<len;) {
		DEBUGADD(level,("%02X ",(int)buf[i]));
		i++;
		if (i%8 == 0) DEBUGADD(level,(" "));
		if (i%16 == 0) {      
			print_asc(level,&buf[i-16],8); DEBUGADD(level,(" "));
			print_asc(level,&buf[i-8],8); DEBUGADD(level,("\n"));
			if (i<len) DEBUGADD(level,("[%03X] ",i));
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

static void *map_file(const char *fname, size_t *size)
{
	int fd = open(fname, O_RDONLY);
	struct stat st;
	void *p;

	if (fd == -1) return MAP_FAILED;

	fstat(fd, &st);
	p = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);

	*size = st.st_size;
	return p;
}

static void file_save(const char *fname, const char *p, off_t size)
{
	int fd;
	fd = open(fname, O_CREAT|O_TRUNC|O_WRONLY, 0666);
	if (fd == -1) {
		perror(fname);
		return;
	}
	if (write(fd, p, size) != size) {
		fprintf(stderr, "Failed to save %d bytes to %s\n", (int)size, fname);
		return;
	}
	close(fd);
}

int main(int argc, const char *argv[])
{
	char outbuf[102400];
	struct z_stream_s zs;
	int ret, i;
	void *buf;
	size_t size;

	buf = map_file(argv[1], &size);
	if (buf == MAP_FAILED) {
		perror(argv[1]);
		exit(1);
	}

	for (i=0;i<size-1;i++) {
		memset(&zs, 0, sizeof(zs));

		zs.avail_in = size - i;
		zs.next_in = i + (char *)buf;
		zs.next_out = outbuf;
		zs.avail_out = sizeof(outbuf);

		ret = inflateInit2(&zs, -15);
		if (ret != Z_OK) {
			fprintf(stderr,"inflateInit2 error %d\n", ret);
			goto failed;
		}

		inflate(&zs, Z_BLOCK);

		if (zs.avail_out != sizeof(outbuf) && sizeof(outbuf)-zs.avail_out > 200) {
			printf("Inflated %u bytes at offset %u (consumed %u bytes)\n", 
			       sizeof(outbuf)-zs.avail_out, i, (size-i) - zs.avail_in);
			dump_data(0, outbuf, sizeof(outbuf)-zs.avail_out);
			if (sizeof(outbuf)-zs.avail_out > 10000) {
				file_save("x.dat", outbuf, sizeof(outbuf)-zs.avail_out);
			}
		}

		inflateEnd(&zs);
	}

	return 0;

failed:
	return -1;
}
