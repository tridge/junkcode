#include "rproxy.h"
#include <zlib.h>

static z_stream c;
static char cbuf[1024];
static int eof;

/* a shim between signature.c and librsync to add zlib compression */

void comp_init(void)
{
	memset(&c, 0, sizeof(c));
	
	deflateInit(&c, Z_DEFAULT_COMPRESSION);
	
	c.next_out = (Bytef *)cbuf;
	c.avail_out = sizeof(cbuf);
}


ssize_t comp_write(ssize_t (*fn)(void *, char *, size_t), 
		   void *private, char *buf, size_t len)
{
	int err;

	c.next_in = (Bytef *)buf;
	c.avail_in = len;

	while (c.avail_in || (sizeof(cbuf) != c.avail_out)) {
		if (sizeof(cbuf) != c.avail_out) {
			if (fn(private, cbuf, sizeof(cbuf)-c.avail_out) != 
			    sizeof(cbuf)-c.avail_out) {
				fprintf(stderr,"Error in deflate\n");
				abort();
			}
			c.next_out = (Bytef *)cbuf;
			c.avail_out = sizeof(cbuf);
		}
		if (c.avail_in) {
			if ((err=deflate(&c, Z_NO_FLUSH) != Z_OK)) {
				fprintf(stderr,"Error2 in deflate\n");
			}
		}
	}

	return len;
}


void comp_flush(ssize_t (*fn)(void *, char *, size_t), void *private)
{
	int err = Z_OK;


	while (err == Z_OK) {
		err = deflate(&c, Z_FINISH);
		if (sizeof(cbuf) != c.avail_out) {
			if (fn(private, cbuf, sizeof(cbuf)-c.avail_out) != 
			    sizeof(cbuf)-c.avail_out) {
				fprintf(stderr,"Error in deflate_flush\n");
				abort();
			}
			c.next_out = (Bytef *)cbuf;
			c.avail_out = sizeof(cbuf);
		}
	}
}


void decomp_init(void)
{
	eof = 0;
	memset(&c, 0, sizeof(c));

	inflateInit(&c);
	c.avail_in = 0;
	c.next_in = (Bytef *)cbuf;
}

ssize_t decomp_read(ssize_t (*fn)(void *, char *, size_t), 
		    void *private, char *buf, size_t len)
{
	c.avail_out = len;
	c.next_out = (Bytef *)buf;

	if (!eof && inflate(&c, Z_NO_FLUSH) == Z_STREAM_END) {
		eof = 1;
	}

	while (c.avail_out && !eof) {
		if ((char *)c.next_in > &cbuf[sizeof(cbuf)/2]) {
			memmove(cbuf, c.next_in, c.avail_in);
			c.next_in = (Bytef *)cbuf;
		}

		if (c.avail_in < sizeof(cbuf)) {
			if (fn(private, (char *)c.next_in + c.avail_in, 1) != 1) {
				fprintf(stderr,"Error in inflate\n");
				abort();
			}
			c.avail_in++;
		}

		if (inflate(&c, Z_NO_FLUSH) == Z_STREAM_END) eof = 1;
	}

	return len - c.avail_out;
}

