#include "rproxy.h"

struct mem_buf {
	char *buf;
	off_t ofs;
	ssize_t length;
};

struct file_buf {
	FILE *f;
	FILE *f_cache;
	ssize_t length;
};


size_t sig_inbytes, sig_outbytes, sig_zinbytes, sig_zoutbytes;

static ssize_t sig_writebuf(void *private, char *buf, size_t len)
{
	struct mem_buf *bofs = (struct mem_buf *)private;

#if DEBUG
	printf("sig_writebuf(len=%d)\n", len);
#endif

	if (bofs->length != -1) {
		len = MIN(len, bofs->length - bofs->ofs);
	}

	memcpy(bofs->buf+bofs->ofs, buf, len);
	bofs->ofs += len;
	return len;
}

static ssize_t sig_readbuf(void *private, char *buf, size_t len)
{
	struct mem_buf *bofs = (struct mem_buf *)private;

#if DEBUG
	printf("sig_readbuf(len=%d)\n", len);
#endif

	if (bofs->length != -1) {
		len = MIN(len, bofs->length - bofs->ofs);
	}

	memcpy(buf, bofs->buf+bofs->ofs, len);
	bofs->ofs += len;
	return len;
}

static ssize_t sig_readfn(void *private, char *buf, size_t len)
{
	struct file_buf *fbuf = (struct file_buf *)private;
	ssize_t n;
	size_t len2;

	if (fbuf->length == 0) {
		n = 0;
		goto out;
	}

	if (fbuf->length == -1) {
		n = fread(buf, 1, len, fbuf->f);
		if (n > 0 && fbuf->f_cache) {
			fwrite(buf, 1, n, fbuf->f_cache);
		}
		goto out;
	}

	len2 = MIN(len, fbuf->length);

	n = fread(buf, 1, len2, fbuf->f);

	if (n <= 0) {
		fbuf->length = 0;
		n = 0;
		goto out;
	}

	fbuf->length -= n;

	if (n > 0 && fbuf->f_cache) {
		fwrite(buf, 1, n, fbuf->f_cache);
	}

 out:
#if DEBUG
	printf("sig_readfn(fd=%d len=%d) -> %d\n", fileno(fbuf->f), len, n);
#endif

	sig_inbytes += n;

	return n;
}

static ssize_t sig_zreadfn(void *private, char *buf, size_t len)
{
	ssize_t ret;

	ret = decomp_read(sig_readfn, private, buf, len);

#if DEBUG
	{
		struct file_buf *fbuf = (struct file_buf *)private;
		printf("sig_zreadfn(fd=%d len=%d) -> %d\n", 
		       fileno(fbuf->f), len, ret);
	}
#endif

	sig_zinbytes += ret;

	return ret;
}

static ssize_t sig_writefn(void *private, char *buf, size_t len)
{
	struct file_buf *fbuf = (struct file_buf *)private;
	ssize_t n;

	n = fwrite(buf, 1, len, fbuf->f);
	if (fbuf->f_cache && n > 0) {
		fwrite(buf, 1, n, fbuf->f_cache);
	}

#if DEBUG
	printf("sig_writefn(fd=%d len=%d)\n", fileno(fbuf->f), len);
#endif

	sig_outbytes += n;

	return n;
}

static ssize_t sig_zwritefn(void *private, char *buf, size_t len)
{
	ssize_t ret;

	ret = comp_write(sig_writefn, private, buf, len);

	sig_zoutbytes += ret;

#if DEBUG
	{
		struct file_buf *fbuf = (struct file_buf *)private;
		printf("sig_zwritefn(fd=%d len=%d) -> %d\n", 
		       fileno(fbuf->f), len, ret);
	}
#endif

	return ret;
}

static ssize_t sig_readfn_ofs(void *private, char *buf, size_t len, off_t ofs)
{
	struct file_buf *fbuf = (struct file_buf *)private;
	ssize_t n;


	fseek(fbuf->f, ofs, SEEK_SET);
	n = fread(buf, 1, len, fbuf->f);

#if DEBUG
	printf("sig_readfn_ofs(fd=%d len=%d ofs=%d) -> %d\n", 
	       fileno(fbuf->f), len, (int)ofs, n);
#endif

	return n;
}


/* generate the signatures for a file putting them in a sigblock,
   and return a pointer to the sigblock, which must be base64 encoded */
char *sig_generate(FILE *f)
{
	struct stat st;
	size_t block_size, sig_size;
	char *buf, *buf2;
	struct mem_buf mbuf;
	struct file_buf fbuf;

	if (fstat(fileno(f), &st)) {
		printf("fstat error in sig_generate\n");
		exit_cleanup(1);
	}

	block_size = (st.st_size / (MAX_SIG_SIZE/(4+4))) & ~15;
	if (block_size < 128) block_size = 128;

	buf  = (char *)xmalloc(MAX_SIG_SIZE*2);
	buf2 = (char *)xmalloc(MAX_SIG_SIZE*2); /* plenty of extra space */

	mbuf.buf = buf;
	mbuf.ofs = 0;
	mbuf.length = -1;

	fbuf.f = f;
	fbuf.f_cache = NULL;
	fbuf.length = -1;

	sig_inbytes = sig_outbytes = sig_zinbytes = sig_zoutbytes = 0;

	sig_size = librsync_signature(&fbuf, 
				      &mbuf, 
				      sig_readfn, 
				      sig_writebuf, 
				      block_size);

	base64_encode((unsigned char *)buf, sig_size, buf2);

#if DEBUG
	printf("sig_generate siglen=%d b64=%d (in=%d out=%d)\n", 
	       sig_size, strlen(buf2),
	       (int)sig_inbytes, (int)sig_outbytes);
#endif

	free(buf);
	return buf2;
}

/* decode a rsync-encoded file stream, putting the decoded data to
   f_out and f_cache. The file used to generate the signature is 
   available in f_orig */
void sig_decode(FILE *f_in, FILE *f_out, FILE *f_orig, FILE *f_cache, 
		ssize_t content_length)
{
	struct file_buf fbuf_orig, fbuf_out, fbuf_in;
	ssize_t n;

	fbuf_orig.f = f_orig;
	fbuf_orig.f_cache = NULL;
	fbuf_orig.length = -1;

	fbuf_out.f = f_out;
	fbuf_out.f_cache = f_cache;
	fbuf_out.length = -1;

	fbuf_in.f = f_in;
	fbuf_in.f_cache = NULL;
	fbuf_in.length = content_length;

	sig_inbytes = sig_outbytes = 0;

	decomp_init();

	n = librsync_decode(&fbuf_orig,
			    &fbuf_out,
			    &fbuf_in,
			    sig_readfn_ofs,
			    sig_writefn,
			    sig_zreadfn);

	printf("librsync_decode->%d (in=%d out=%d)\n", 
	       n, (int)sig_inbytes, (int)sig_outbytes);
}


/* encode a file stream, putting the encoded data to f_out and the
   unencoded data to f_cache. The signature of the original file is in
   signature, in base64 form */
void sig_encode(FILE *f_in, FILE *f_out, char *signature, FILE *f_cache,
		ssize_t content_length)
{
	struct file_buf fbuf_r, fbuf_w;
	struct mem_buf mbuf;
	char *sig2;
	size_t sig_length;
	ssize_t n;

	sig2 = xstrdup(signature);
	sig_length = base64_decode(sig2);

	fbuf_r.f = f_in;
	fbuf_r.f_cache = f_cache;
	fbuf_r.length = content_length;

	printf("sig_encode content_length=%d\n", content_length);

	fbuf_w.f = f_out;
	fbuf_w.f_cache = NULL;
	fbuf_w.length = -1;

	mbuf.buf = sig2;
	mbuf.ofs = 0;
	mbuf.length = sig_length;

	sig_inbytes = sig_outbytes = 0;

	comp_init();

	n = librsync_encode((void *)&fbuf_r,
			    (void *)&fbuf_w,
			    (void *)&mbuf, 
			    sig_readfn,
			    sig_zwritefn,
			    sig_readbuf);

	comp_flush(sig_writefn, (void *)&fbuf_w);

	free(sig2);
	printf("librsync_encode->%d (in=%d out=%d)\n", 
	       n, (int)sig_inbytes, (int)sig_outbytes);
}
