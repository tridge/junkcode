#include <stdio.h>
#include <iconv.h>
#include <errno.h>
#include <sys/time.h>

struct timeval tp1,tp2;

static void start_timer()
{
	gettimeofday(&tp1,NULL);
}

static double end_timer()
{
	gettimeofday(&tp2,NULL);
	return((tp2.tv_sec - tp1.tv_sec) + 
	       (tp2.tv_usec - tp1.tv_usec)*1.0e-6);
}

static size_t utf8_pull(char **inbuf, size_t *inbytesleft,
			 char **outbuf, size_t *outbytesleft)
{
	while (*inbytesleft >= 1 && *outbytesleft >= 2) {
		unsigned char *c = (unsigned char *)*inbuf;
		unsigned char *uc = (unsigned char *)*outbuf;
		int len = 1;

		if ((c[0] & 0x80) == 0) {
			uc[0] = c[0];
			uc[1] = 0;
		} else if ((c[0] & 0xf0) == 0xe0) {
			if (*inbytesleft < 3) {
				goto badseq;
			}
			uc[1] = ((c[0]&0xF)<<4) | ((c[1]>>2)&0xF);
			uc[0] = (c[1]<<6) | (c[2]&0x3f);
			len = 3;
		} else if ((c[0] & 0xe0) == 0xc0) {
			if (*inbytesleft < 2) {
				goto badseq;
			}
			uc[1] = (c[0]>>2) & 0x7;
			uc[0] = (c[0]<<6) | (c[1]&0x3f);
			len = 2;
		}

		(*inbuf)  += len;
		(*inbytesleft)  -= len;
		(*outbytesleft) -= 2;
		(*outbuf) += 2;
	}

	if (*inbytesleft > 0) {
		errno = E2BIG;
		return -1;
	}
	
	return 0;

badseq:
	errno = EINVAL;
	return -1;
}

static size_t utf8_push(char **inbuf, size_t *inbytesleft,
			 char **outbuf, size_t *outbytesleft)
{
	while (*inbytesleft >= 2 && *outbytesleft >= 1) {
		unsigned char *c = (unsigned char *)*outbuf;
		unsigned char *uc = (unsigned char *)*inbuf;
		int len=1;

		if ((uc[1] & 0xf8) == 0xd8) {
			if (*outbytesleft < 3) {
				goto toobig;
			}
			c[0] = 0xed;
			c[1] = 0x9f;
			c[2] = 0xbf;
			len = 3;
		} else if (uc[1] & 0xf8) {
			if (*outbytesleft < 3) {
				goto toobig;
			}
			c[0] = 0xe0 | (uc[1]>>4);
			c[1] = 0x80 | ((uc[1]&0xF)<<2) | (uc[0]>>6);
			c[2] = 0x80 | (uc[0]&0x3f);
			len = 3;
		} else if (uc[1] | (uc[0] & 0x80)) {
			if (*outbytesleft < 2) {
				goto toobig;
			}
			c[0] = 0xc0 | (uc[1]<<2) | (uc[0]>>6);
			c[1] = 0x80 | (uc[0]&0x3f);
			len = 2;
		} else {
			c[0] = uc[0];
		}


		(*outbuf)[0] = (*inbuf)[0];
		(*inbytesleft)  -= 2;
		(*outbytesleft) -= len;
		(*inbuf)  += 2;
		(*outbuf) += len;
	}

	if (*inbytesleft == 1) {
		errno = EINVAL;
		return -1;
	}

	if (*inbytesleft > 1) {
		errno = E2BIG;
		return -1;
	}
	
	return 0;

toobig:
	errno = E2BIG;
	return -1;
}




int main(int argc, char *argv[])
{
	iconv_t cd;
	char *test_string, *buf1, *buf2;
	int buflen, string_len;
	int i;
	int len1, len2;

	cd = iconv_open("UCS2", argv[1]);
	test_string = argv[2];

	string_len = strlen(test_string);
	buflen = strlen(test_string)*5;
	buf1 = malloc(buflen);
	buf2 = malloc(buflen);

	start_timer();

	for (i=0; end_timer() < 1; i++) {
		char *p = test_string;
		char *q = buf1;
		size_t inbytes = string_len;
		size_t outbytes = buflen;

		iconv(cd, &p, &inbytes, &q, &outbytes);
		len1 = buflen - outbytes;

	}

	printf("iconv: %g ops/sec\n", i/end_timer());

	start_timer();

	for (i=0; end_timer() < 1; i++) {
		char *p = test_string;
		char *q = buf2;
		size_t inbytes = string_len;
		size_t outbytes = buflen;

		utf8_pull(&p, &inbytes, &q, &outbytes);
		len2 = buflen - outbytes;		
	}

	printf("internal: %g ops/sec\n", i/end_timer());
	printf("string_len=%d len1=%d len2=%d\n", string_len, len1, len2);

	if (len1 != len2 || memcmp(buf1, buf2, len1) != 0) {
		printf("results DIFFER! (%10.10s) (%10.10s)\n",
		       buf1, buf2);
	}

	return 0;
}
