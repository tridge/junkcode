#include "rproxy.h"

char *body_load(FILE *f, int *content_length)
{
	int alloc_length=0;
	char *body = NULL;

	(*content_length) = header_ival("Content-Length");
	
	if ((*content_length) != -1) {
		body = xmalloc(*content_length);
		if (read_all(f, body, *content_length) != *content_length) {
			errmsg("Failed to read complete body\n");
			exit(1);
		}
		return body;
	}

	(*content_length) = 0;
	
	/* we need to read to eof, allocating on the way */
	while (1) {
		int n, size = 0x1000;
		if ((*content_length) + size > alloc_length) {
			body = xrealloc(body, (*content_length) + size);
		}
		n = fread(body + (*content_length), 1, size, f);
		if (n <= 0) break;
		(*content_length) += n;
	}

	return body;
}

ssize_t read_write(FILE *f_in, FILE *f_out, FILE *f_copy, size_t size)
{
	char buf[1024];
	ssize_t total = 0;

	while (size) {
		ssize_t n = fread(buf, 1, MIN(sizeof(buf),size), f_in);
		if (n <= 0) return total;
		fwrite(buf, 1, n, f_out);
		if (f_copy) fwrite(buf, 1, n, f_copy);
		total += n;
		size -= n;
	}
	return total;
}

/* stream data from f_in to f_out, also sending a copy to f_copy if not NULL */
size_t stream_body(FILE *f_in, FILE *f_out, FILE *f_copy, int content_length)
{
	ssize_t n = 0;
	
	if (content_length != -1) {
		n = read_write(f_in, f_out, f_copy, content_length);
	} else {
		int s;
		while ((s = read_write(f_in, f_out, f_copy, 1024))) n += s;
	}

	fflush(f_out);
	fflush(f_copy);

	logmsg("streamed %d bytes\n", n);
	return n;
}

