/*
  this is an attempted implementation of Broders shingles algorithm for similarity testing of documents

  see http://gatekeeper.dec.com/pub/DEC/SRC/publications/broder/positano-final-wpnums.pdf

  tridge@samba.org
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

#define MAX_WORD_SIZE 8
#define W_SHINGLES 10
#define SKETCH_SIZE 32

#define HASH_PRIME 0x01000193
#define HASH_INIT 0x28021967

typedef unsigned u32;
typedef unsigned short u16;
typedef unsigned char uchar;

/* a simple non-rolling hash, based on the FNV hash */
static inline u32 sum_hash(uchar c, u32 h)
{
	h *= HASH_PRIME;
	h ^= c;
	return h;
}

static uchar *advance(uchar *in, unsigned w, uchar *limit)
{
	in++;
	while (w--) {
		int i;
		for (i=0;i<MAX_WORD_SIZE;i++) {
			if (in >= limit || isspace(in[i])) break;
		}
		in += i;
	}
	return in;
}

static u16 hash_emit(uchar *start, size_t len)
{
	u32 v = HASH_INIT;

	while (len--) {
		if (isalnum(*start)) {
			v = sum_hash(*start, v);
		}
		start++;
	}
	return v % 4096;
}

static int u32_comp(u32 *v1, u32 *v2)
{
	return *v1 - *v2;
}

static uchar *shingle_sketch(uchar *in_0, size_t size)
{
	uchar *in = in_0;
	uchar *p;
	u16 *sketch;
	uchar *ret;
	u32 sketch_size = 0;
	u32 rands[SKETCH_SIZE];
	int i;
	const char *b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	/* allocate worst case size */
	sketch = malloc(size*2);
	ret = malloc(SKETCH_SIZE*2 + 1);

	p = advance(in, W_SHINGLES, in_0 + size);

	sketch[sketch_size++] = hash_emit(in, p-in);
	while (p < in_0 + size) {
		in = advance(in, 1, in_0 + size);
		p = advance(p, 1, in_0 + size);
		sketch[sketch_size++] = hash_emit(in, p-in);
	}

	srandom(time(NULL));
	for (i=0;i<SKETCH_SIZE;i++) {
		rands[i] = random() % sketch_size;
	}
	qsort(rands, SKETCH_SIZE, sizeof(rands[0]), u32_comp);
	
	for (i=0;i<SKETCH_SIZE;i++) {
		u16 v = sketch[rands[i]];
		ret[2*i] = b64[v % 64];
		ret[2*i+1] = b64[(v>>6) % 64];
	}
	ret[2*i] = 0;

	return ret;
}

/*
  return the sketch of a file
*/
uchar *sketch_file(char *fname)
{
	int fd;
	struct stat st;
	uchar *msg;
	uchar *ret;

	fd = open(fname, O_RDONLY);
	if (fd == -1) {
		perror(fname);
		return NULL;
	}

	if (fstat(fd, &st) == -1) {
		perror("fstat");
		return NULL;
	}

	msg = mmap(NULL, st.st_size, PROT_READ, MAP_FILE|MAP_PRIVATE, fd, 0);
	if (msg == (uchar *)-1) {
		perror("mmap");
		return NULL;
	}
	close(fd);

	ret = shingle_sketch(msg, st.st_size);

	munmap(msg, st.st_size);
	
	return ret;
}

int main(int argc, char *argv[])
{
	uchar *ret;

	ret = sketch_file(argv[1]);

	printf("%s\n", ret);

	return 0;
}
