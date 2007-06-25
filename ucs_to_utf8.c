#include <stdio.h>
#include <iconv.h>

static int utf8_encode(unsigned char c[4], unsigned uc)
{
	unsigned char uc1, uc2;
	uc1 = uc & 0xFF;
	uc2 = uc >> 8;

	if ((uc2 & 0xf8) == 0xd8) {
		c[0] = 0xed;
		c[1] = 0x9f;
		c[2] = 0xbf;
		c[3] = 0;
		return 3;
	}

	if (uc2 & 0xf8) {
		c[0] = 0xe0 | (uc2>>4);
		c[1] = 0x80 | ((uc2&0xF)<<2) | (uc1>>6);
		c[2] = 0x80 | (uc1&0x3f);
		c[3] = 0;
		return 3;
	}

	if (uc2 | (uc1 & 0x80)) {
		c[0] = 0xc0 | (uc2<<2) | (uc1>>6);
		c[1] = 0x80 | (uc1&0x3f);
		c[2] = 0;
		return 2;
	} 

	c[0] = uc1;
	c[1] = 0;

	return 1;
}

int main(int argc, char *argv[])
{
	int i;
	

	for (i=1;i<argc;i++) {
		char s[4];
		unsigned c;

		c = strtol(argv[i], NULL, 16);

		int len = utf8_encode(s, c);

		s[len] = '\n';
		write(1, s, len+1);
	}

	return 0;
}
