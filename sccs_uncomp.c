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


static int uncompress_one(const char *fname)
{
	unsigned char *buf;
	size_t size;
	unsigned char *p;
	struct z_stream_s zs;
	int ret, len;
	char outbuf[1024];
	char *tmpname;
	FILE *f;
	unsigned sum;
	
	buf = map_file(fname, &size);

	p = memmem(buf, size, "\1f e 4\n", 7);
	if (p == NULL) {
		p = memmem(buf, size, "\1f e 5\n", 7);
	}
	if (p == NULL) {
		printf("%s not compressed (no e 4)\n", fname);
		return 0;
	}

	if (p[5] == '4') {
		p[5] = '0';
	} else {
		p[5] = '1';
	}

	p = memmem(buf, size, "\1T\nx", 4);

	if (p == NULL) {
		printf("%s not compressed\n", fname);
		return 0;
	}

	asprintf(&tmpname, "%s.tmpz", fname);
	
	f = fopen(tmpname, "w");

	len = 3+(int)(p-buf);
	fwrite(buf, 1, len, f);

	memset(&zs, 0, sizeof(zs));

	zs.avail_in = size-len-2;
	zs.next_in = p+5;
	zs.next_out = outbuf;
	zs.avail_out = sizeof(outbuf);

	ret = inflateInit2(&zs, -15);
	if (ret != Z_OK) {
		fprintf(stderr,"inflateInit2 error %d\n", ret);
		goto failed;
	}

	while (1) {
		if (zs.avail_in == 0) break;

		ret = inflate(&zs, Z_SYNC_FLUSH);

		if (zs.avail_out < sizeof(outbuf)) {
			fwrite(outbuf, 1, sizeof(outbuf) - zs.avail_out, f);
			zs.avail_out = sizeof(outbuf);
			zs.next_out = outbuf;
		}

		if (ret == Z_STREAM_END) break;
		if (ret != Z_OK) {
			fprintf(stderr,"inflate error %d\n", ret);
			goto failed;
		}
	}

	fflush(f);
	rewind(f);

	buf = map_file(tmpname, &size);

	sum = sccs_checksum(buf+8, size-8);

	printf("sum=%05u\n", sum);

	fprintf(f, "\1H%05u", sum);
	fclose(f);

	rename(tmpname, fname);
	
	return 0;

failed:
	printf("failed\n");
	return -1;
}


int main(int argc, const char *argv[])
{
	int i;
	for (i=1;i<argc;i++) {
		if (uncompress_one(argv[i]) != 0) return -1;
	}
	return 0;
}
