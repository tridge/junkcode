#include <stdio.h>
#include <iconv.h>

#define CVAL(buf,pos) (((unsigned char *)(buf))[pos])
#define SVAL(buf, pos) (CVAL(buf, pos) | CVAL(buf, pos+1)<<8)

typedef unsigned short uint16;
#define smb_ucs2_t unsigned short
#define UNI_UPPER    0x1
#define UNI_LOWER    0x2
#define UNI_DIGIT    0x4
#define UNI_XDIGIT   0x8
#define UNI_SPACE    0x10

typedef struct {
	smb_ucs2_t lower;
	smb_ucs2_t upper;
	unsigned char flags;
} smb_unicode_table_t;

smb_unicode_table_t map_table[] = {
#include "unicode_map_table.h"
};

static int zero_len;
static int error_count;

/* check if a converted character contains a null */
static void check_null(iconv_t cd_in, iconv_t cd_out, smb_ucs2_t uc)
{
	char buf[10];
	int inlen=2, outlen=10;
	char *p = (char *)&uc;
	char *q = buf;
	int i, len;

	iconv(cd_in, &p, &inlen, &q, &outlen);
	
	if (outlen == 10) {
		zero_len++;
		return;
	}

	len = (10-outlen);

	for (i=0;i<len;i++) {
		if (buf[i] == 0) {
			printf("NULL at pos %d in ucs2 char %04x\n",
			       i, uc);
		}
	}
}

/* check if a uppercase or lowercase version of a char is longer than
   the original */
static void check_case(iconv_t cd_in, iconv_t cd_out, smb_ucs2_t uc)
{
	char buf[10];
	int inlen=2, outlen=10;
	char *p = (char *)&uc;
	char *q = buf;
	int len1, len2, len3;
	smb_ucs2_t uc2;

	iconv(cd_in, &p, &inlen, &q, &outlen);
	len1 = (10-outlen);

	if (len1 == 0) return;

	uc2 = map_table[uc].upper;
	p = (char *)&uc2;
	q = buf;
	inlen = 2;
	outlen = 10;

	iconv(cd_in, &p, &inlen, &q, &outlen);
	len2 = (10-outlen);

	uc2 = map_table[uc].lower;
	p = (char *)&uc2;
	q = buf;
	inlen = 2;
	outlen = 10;

	iconv(cd_in, &p, &inlen, &q, &outlen);
	len3 = (10-outlen);

	if (len2 > len1 || len3 > len1) {
		printf("case expansion for ucs2 char %04x (%d/%d/%d)\n",
		       uc, len1, len2, len3);
		error_count++;
	}
}


/* check if a character is C compatible */
static void check_compat(iconv_t cd_in, iconv_t cd_out, smb_ucs2_t uc)
{
	char buf[10];
	int inlen, outlen;
	char *p, *q;
	smb_ucs2_t uc2;

	/* only care about 7 bit chars */
	if (SVAL(&uc, 0) & 0xFF80) return;

	inlen = 2; outlen = 10;
	p = (char *)&uc;
	q = buf;
	iconv(cd_in, &p, &inlen, &q, &outlen);

	if (outlen != 9) {
		printf("ucs2 char %04x not C compatible (len=%d)\n",
		       uc, 10-outlen);
		return;
	}

	if (buf[0] != SVAL(&uc, 0)) {
		printf("ucs2 char %04x not C compatible (c=0x%x)\n",
		       uc, buf[0]);
		error_count++;
		return;
	}

	inlen = 1; outlen = 2;
	p = (char *)&uc2;
	q = buf;
	iconv(cd_out, &q, &inlen, &p, &outlen);
	
	if (uc2 != uc) {
		printf("ucs2 char %04x not C compatible (uc2=0x%x len=%d): %s\n",
		       uc, uc2, 2-outlen, strerror(errno));
		error_count++;
	}
}

/* check if a character is strchr compatible */
static void check_strchr_compat(iconv_t cd_in, iconv_t cd_out, smb_ucs2_t uc)
{
	char buf[10];
	int inlen=2, outlen=10;
	int i;
	char *p = (char *)&uc;
	char *q = buf;

	iconv(cd_in, &p, &inlen, &q, &outlen);

	if (10 - outlen <= 1) return;

	for (i=0; i<10-outlen; i++) {
		if (! (buf[i] & 0x80)) {
			printf("ucs2 char %04x not strchr compatible\n",
			       uc);
			error_count++;
		}
	}
}

int main(int argc, char *argv[])
{
	int i;
	iconv_t cd_in, cd_out;

	if (argc < 2) {
		printf("Usage: charset_test <CHARSET>\n");
		exit(1);
	}

	cd_in = iconv_open(argv[1], "UCS-2LE");
	cd_out = iconv_open("UCS-2LE", argv[1]);

	if (cd_in == (iconv_t)-1 || cd_out == (iconv_t)-1) {
		perror(argv[1]);
		return -1;
	}

	for (i=1;i<0x10000;i++) {
		iconv(cd_in, NULL, NULL, NULL, NULL);
		iconv(cd_out, NULL, NULL, NULL, NULL);
		check_null(cd_in, cd_out, i);
		check_case(cd_in, cd_out, i);
		check_compat(cd_in, cd_out, i);
		/* check_strchr_compat(cd_in, cd_out, i); */
	}

	printf("%d chars convertible\n", 0x10000 - zero_len);
	if (error_count == 0) {
		printf("character set %s OK for Samba\n", argv[1]);
	} else {
		printf("%d errors in character set\n", error_count);
		return -1;
	}
	return 0;
}
