#include <stdio.h>
#include <iconv.h>

typedef unsigned short smb_ucs2_t;

#define CVAL(buf,pos) (((unsigned char *)(buf))[pos])

static int utf8_encode(unsigned char c[3], smb_ucs2_t uc)
{
	unsigned char uc1, uc2;
	uc1 = CVAL(&uc, 0);
	uc2 = CVAL(&uc, 1);

	if ((uc2 & 0xf8) == 0xd8) {
		c[0] = 0xed;
		c[1] = 0x9f;
		c[2] = 0xbf;
		return 3;
	}

	if (uc2 & 0xf8) {
		c[0] = 0xe0 | (uc2>>4);
		c[1] = 0x80 | ((uc2&0xF)<<2) | (uc1>>6);
		c[2] = 0x80 | (uc1&0x3f);
		return 3;
	}

	if (uc2 | (uc1 & 0x80)) {
		c[0] = 0xc0 | (uc2<<2) | (uc1>>6);
		c[1] = 0x80 | (uc1&0x3f);
		return 2;
	} 

	c[0] = uc1;
	return 1;
}

static int utf8_decode(unsigned char c[3], smb_ucs2_t *uc)
{
	*uc = 0;

	if ((c[0] & 0xf0) == 0xe0) {
		CVAL(uc, 1) = ((c[0]&0xF)<<4) | ((c[1]>>2)&0xF);
		CVAL(uc, 0) = (c[1]<<6) | (c[2]&0x3f);
		return 3;
	}

	if ((c[0] & 0xe0) == 0xc0) {
		CVAL(uc, 1) = (c[0]>>2) & 0x7;
		CVAL(uc, 0) = (c[0]<<6) | (c[1]&0x3f);
		return 2;
	}

	CVAL(uc, 0) = c[0];
	return 1;
}


int main(int argc, char *argv[])
{
	unsigned short s[2];
	int i, j;
	unsigned char utf8[10];
	unsigned char foo[10];
	int len, len2;
	iconv_t cd;

	s[1] = 0;

	cd = iconv_open(argv[1], "UCS2");
	
	for (i=1;i<0x10000;i++) {
		int inlen, outlen;
		char *p;
		char *q;

		p = s;
		q = utf8;
		s[0] = i;
		inlen = 4;
		outlen = 10;
		iconv(cd, &p, &inlen, &q, &outlen);
		len = strlen(utf8);
		len2 = utf8_encode(foo, i);

		if (1 || len != len2 || strncmp(foo, utf8, len) != 0) {
			printf("%02x: ", i);
			for (j=0;j<len;j++) {
				printf("%02x ", utf8[j]);
			}
			printf("\n");
			
			printf("%02x: ", i);
			for (j=0;j<len2;j++) {
				printf("%02x ", foo[j]);
			}
			printf("\n");
		}
	}
}
