#include <stdio.h>
#include <errno.h>
#include <iconv.h>

typedef unsigned short smb_ucs2_t;

#define CVAL(buf,pos) (((unsigned char *)(buf))[pos])
#define DEBUG(x, s) printf s

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
				DEBUG(0,("short utf8 char\n"));
				goto badseq;
			}
			uc[1] = ((c[0]&0xF)<<4) | ((c[1]>>2)&0xF);
			uc[0] = (c[1]<<6) | (c[2]&0x3f);
			len = 3;
		} else if ((c[0] & 0xe0) == 0xc0) {
			if (*inbytesleft < 2) {
				DEBUG(0,("short utf8 char\n"));
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
				DEBUG(0,("short utf8 write\n"));
				goto toobig;
			}
			c[0] = 0xed;
			c[1] = 0x9f;
			c[2] = 0xbf;
			len = 3;
		} else if (uc[1] & 0xf8) {
			if (*outbytesleft < 3) {
				DEBUG(0,("short utf8 write\n"));
				goto toobig;
			}
			c[0] = 0xe0 | (uc[1]>>4);
			c[1] = 0x80 | ((uc[1]&0xF)<<2) | (uc[0]>>6);
			c[2] = 0x80 | (uc[0]&0x3f);
			len = 3;
		} else if (uc[1] | (uc[0] & 0x80)) {
			if (*outbytesleft < 2) {
				DEBUG(0,("short utf8 write\n"));
				goto toobig;
			}
			c[0] = 0xc0 | (uc[1]<<2) | (uc[0]>>6);
			c[1] = 0x80 | (uc[0]&0x3f);
			len = 2;
		} else {
			c[0] = uc[0];
		}


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



main()
{
	smb_ucs2_t s[2] = {0xe4, 0};
	iconv_t cd;
	unsigned char buf[10];
	int i;
	char *inp, *outp;
	int inlen, outlen;

	printf("Converting 0x%04x\n", s[0]);

	cd = iconv_open("UTF8", "UCS2");

	inp = s;
	inlen = 2;
	outp = buf;
	outlen = 10;

	utf8_push(&inp, &inlen, &outp, &outlen);

	for (i=0;i<10-outlen;i++) {
		printf("%02x ", buf[i]);
	}
	printf("\n");

	inp = s;
	inlen = 2;
	outp = buf;
	outlen = 10;

	iconv(cd, &inp, &inlen, &outp, &outlen);

	for (i=0;i<10-outlen;i++) {
		printf("%02x ", buf[i]);
	}
	printf("\n");
}
