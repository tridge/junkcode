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
	char buf[8192];
	void *fn;
};


size_t sig_inbytes, sig_outbytes, sig_zinbytes, sig_zoutbytes;

static ssize_t sig_writebuf(void *private, char *buf, size_t len)
{
	struct mem_buf *bofs = (struct mem_buf *)private;

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
	sig_inbytes += n;

	return n;
}


static ssize_t sig_zreadfn(void *private, char *buf, size_t len)
{
	ssize_t ret;

	ret = decomp_read(sig_readfn, private, buf, len);

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

	sig_outbytes += n;

	return n;
}

static ssize_t sig_zwritefn(void *private, char *buf, size_t len)
{
	ssize_t ret;

	ret = comp_write(sig_writefn, private, buf, len);

	sig_zoutbytes += ret;

	return ret;
}

static ssize_t sig_readfn_ofs(void *private, char *buf, size_t len, off_t ofs)
{
	struct file_buf *fbuf = (struct file_buf *)private;
	ssize_t n;


	fseek(fbuf->f, ofs, SEEK_SET);
	n = fread(buf, 1, len, fbuf->f);

	return n;
}


static rs_result rsig_readfn(rs_job_t *job, rs_buffers_t *buf,
			     void *opaque)
{
	struct file_buf *fbuf = (struct file_buf *)opaque;
	int len;

	if (buf->eof_in) {
		return RS_DONE;
	}

	if (buf->avail_in) {
		return RS_DONE;
	}

	len = sig_readfn(fbuf, fbuf->buf, sizeof(fbuf->buf));
	if (len == 0) {
		buf->eof_in = 1;
		return RS_DONE;
	}
	if (len == -1) {
            return RS_IO_ERROR;
	}

	buf->avail_in = len;
	buf->next_in = fbuf->buf;

	return RS_DONE;
}


static rs_result rsig_zreadfn(rs_job_t *job, rs_buffers_t *buf,
			     void *opaque)
{
	struct file_buf *fbuf = (struct file_buf *)opaque;
	int len;

	if (buf->eof_in) {
		return RS_DONE;
	}

	if (buf->avail_in) {
		return RS_DONE;
	}

	len = sig_zreadfn(fbuf, fbuf->buf, sizeof(fbuf->buf));
	if (len == 0) {
		buf->eof_in = 1;
		return RS_DONE;
	}
	if (len == -1) {
            return RS_IO_ERROR;
	}

	buf->avail_in = len;
	buf->next_in = fbuf->buf;

	return RS_DONE;
}


rs_result rsig_writebuf(rs_job_t *job, rs_buffers_t *buf, void *opaque)
{
	struct mem_buf *mbuf = (struct mem_buf *)opaque;

	if (buf->next_out == NULL) {
		buf->next_out = mbuf->buf;
		buf->avail_out = mbuf->length;
		return RS_DONE;
	}
        
	mbuf->ofs = buf->next_out - mbuf->buf;
	if (mbuf->ofs >= mbuf->length) {
		return RS_IO_ERROR;
	}

	buf->avail_out = mbuf->length - mbuf->ofs;

	return RS_DONE;
}


/* generate the signatures for a file putting them in a sigblock,
   and return a pointer to the sigblock, which must be base64 encoded */
char *sig_generate(FILE *f)
{
	rs_buffers_t rbuf;
	struct stat st;
	size_t block_size;
	char *buf, *buf2;
	struct mem_buf mbuf;
	struct file_buf fbuf;
	rs_job_t *job;
	rs_result r;

	if (fstat(fileno(f), &st)) {
		errmsg("fstat error in sig_generate\n");
		exit_cleanup(1);
	}

	block_size = (st.st_size / (MAX_SIG_SIZE/(8+8))) & ~15;
	if (block_size < 128) block_size = 128;

	buf  = (char *)xmalloc(MAX_SIG_SIZE*2);
	buf2 = (char *)xmalloc(MAX_SIG_SIZE*2); /* plenty of extra space */

	mbuf.buf = buf;
	mbuf.ofs = 0;
	mbuf.length = MAX_SIG_SIZE*2;

	fbuf.f = f;
	fbuf.f_cache = NULL;
	fbuf.length = -1;

	sig_inbytes = sig_outbytes = sig_zinbytes = sig_zoutbytes = 0;

	memset(&rbuf, 0, sizeof(rbuf));

	job = rs_sig_begin(block_size, 8);
	r = rs_job_drive(job, &rbuf,
			 rsig_readfn, (void *)&fbuf,
			 rsig_writebuf, (void *)&mbuf);
	rs_job_free(job);

	base64_encode((unsigned char *)buf, mbuf.ofs, buf2);

	logmsg("generated signature of size %u block_size=%d inbytes=%d\n", 
	       (unsigned)mbuf.ofs, block_size, sig_inbytes);

	free(buf);
	return buf2;
}


static rs_result rsig_readfn_ofs(void *opaque, off_t pos,
				 size_t *len, void **buf)
{
	int ret;

	ret = sig_readfn_ofs(opaque, *buf, *len, pos);
	
	if (ret == -1) {
		return RS_IO_ERROR;
	}

	*len = ret;
	return RS_DONE;
}

rs_result rsig_writefn(rs_job_t *job, rs_buffers_t *buf, void *opaque)
{
	struct file_buf *fbuf = (struct file_buf *)opaque;
	int present;

	if (buf->next_out == NULL) {
		buf->next_out = fbuf->buf;
		buf->avail_out = sizeof(fbuf->buf);
		return RS_DONE;
	}

	present = buf->next_out - fbuf->buf;

	if (present > 0) {
		int ret = sig_writefn(fbuf, fbuf->buf, present);
	}

	buf->next_out = fbuf->buf;
	buf->avail_out = sizeof(fbuf->buf);

	return RS_DONE;
}

rs_result rsig_zwritefn(rs_job_t *job, rs_buffers_t *buf, void *opaque)
{
	struct file_buf *fbuf = (struct file_buf *)opaque;
	int present;

	if (buf->next_out == NULL) {
		buf->next_out = fbuf->buf;
		buf->avail_out = sizeof(fbuf->buf);
		return RS_DONE;
	}

	present = buf->next_out - fbuf->buf;

	if (present > 0) {
		int ret = sig_zwritefn(fbuf, fbuf->buf, present);
	}

	buf->next_out = fbuf->buf;
	buf->avail_out = sizeof(fbuf->buf);

	return RS_DONE;
}

/* decode a rsync-encoded file stream, putting the decoded data to
   f_out and f_cache. The file used to generate the signature is 
   available in f_orig */
void sig_decode(FILE *f_in, FILE *f_out, FILE *f_orig, FILE *f_cache, 
		ssize_t content_length)
{
	rs_buffers_t rbuf;
	struct file_buf fbuf_orig, fbuf_out, fbuf_in;
	ssize_t n;
	rs_job_t *job;
	rs_result r;

	fbuf_orig.f = f_orig;
	fbuf_orig.f_cache = NULL;
	fbuf_orig.length = -1;
	fbuf_orig.fn = sig_readfn_ofs;

	fbuf_out.f = f_out;
	fbuf_out.f_cache = f_cache;
	fbuf_out.length = -1;
	fbuf_out.fn = sig_writefn;

	fbuf_in.f = f_in;
	fbuf_in.f_cache = NULL;
	fbuf_in.length = content_length;
	fbuf_in.fn = sig_readfn;

	sig_inbytes = sig_outbytes = 0;

	memset(&rbuf, 0, sizeof(rbuf));

	decomp_init();

	job = rs_patch_begin(rsig_readfn_ofs, &fbuf_orig);
	r = rs_job_drive(job, &rbuf,
			 rsig_zreadfn, (void *)&fbuf_in,
			 rsig_writefn, (void *)&fbuf_out);
	rs_job_free(job);

	logmsg("librsync_decode->%d (in=%d out=%d)\n", 
	       n, (int)sig_inbytes, (int)sig_outbytes);
}


rs_result rsig_readbuf(rs_job_t *job, rs_buffers_t *buf, void *opaque)
{
	struct mem_buf *mbuf = (struct mem_buf *)opaque;

	if (buf->next_in == NULL) {
		buf->next_in = mbuf->buf;
		buf->avail_in = mbuf->length;
		return RS_DONE;
	}

	if (buf->avail_in == 0) {
		buf->eof_in = 1;
	}
        
	return RS_DONE;
}




/* encode a file stream, putting the encoded data to f_out and the
   unencoded data to f_cache. The signature of the original file is in
   signature, in base64 form */
void sig_encode(FILE *f_in, FILE *f_out, char *signature, FILE *f_cache,
		ssize_t content_length)
{
	rs_buffers_t rbuf;
	struct file_buf fbuf_r, fbuf_w;
	struct mem_buf mbuf;
	char *sig2;
	size_t sig_length;
	ssize_t n;
	rs_job_t *job;
	rs_signature_t *sig;
	rs_result r;

	sig2 = xstrdup(signature);
	sig_length = base64_decode(sig2);

	fbuf_r.f = f_in;
	fbuf_r.f_cache = f_cache;
	fbuf_r.length = content_length;

	logmsg("sig_encode content_length=%d sig_length=%u\n", 
	       content_length, sig_length);

	fbuf_w.f = f_out;
	fbuf_w.f_cache = NULL;
	fbuf_w.length = -1;

	mbuf.buf = sig2;
	mbuf.ofs = 0;
	mbuf.length = sig_length;

	sig_inbytes = sig_outbytes = 0;

	memset(&rbuf, 0, sizeof(rbuf));

	job = rs_loadsig_begin(&sig);
	r = rs_job_drive(job, &rbuf,
			 rsig_readbuf, (void *)&mbuf,
			 NULL, NULL);

	rs_job_free(job);

	memset(&rbuf, 0, sizeof(rbuf));

	r = rs_build_hash_table(sig);

	comp_init();

	job = rs_delta_begin(sig);
	r = rs_job_drive(job, &rbuf,
			 rsig_readfn, (void *)&fbuf_r,
			 rsig_zwritefn, (void *)&fbuf_w);
	rs_job_free(job);

	comp_flush(sig_writefn, (void *)&fbuf_w);

	free(sig2);
	logmsg("librsync_encode->%d (in=%d out=%d)\n", 
	       n, (int)sig_inbytes, (int)sig_outbytes);
}
