#include <stdio.h>
#include <iconv.h>

#define CVAL(buf,pos) (((unsigned char *)(buf))[pos])

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
static void check_null(iconv_t cd, smb_ucs2_t uc)
{
	char buf[10];
	int inlen=2, outlen=10;
	char *p = (char *)&uc;
	char *q = buf;
	int i, len;

	iconv(cd, &p, &inlen, &q, &outlen);
	
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
static void check_case(iconv_t cd, smb_ucs2_t uc)
{
	char buf[10];
	int inlen=2, outlen=10;
	char *p = (char *)&uc;
	char *q = buf;
	int len1, len2, len3;
	smb_ucs2_t uc2;

	iconv(cd, &p, &inlen, &q, &outlen);
	len1 = (10-outlen);

	if (len1 == 0) return;

	uc2 = map_table[uc].upper;
	p = (char *)&uc2;
	q = buf;
	inlen = 2;
	outlen = 10;

	iconv(cd, &p, &inlen, &q, &outlen);
	len2 = (10-outlen);

	uc2 = map_table[uc].lower;
	p = (char *)&uc2;
	q = buf;
	inlen = 2;
	outlen = 10;

	iconv(cd, &p, &inlen, &q, &outlen);
	len3 = (10-outlen);

	if (len2 > len1 || len3 > len1) {
		printf("case expansion for ucs2 char %04x (%d/%d/%d)\n",
		       uc, len1, len2, len3);
		error_count++;
	}
}


/* check if a character is C compatible */
static void check_compat(iconv_t cd, smb_ucs2_t uc)
{
	char buf[10];
	int inlen=2, outlen=10;
	char *p = (char *)&uc;
	char *q = buf;

	/* only care about 7 bit chars */
	if (CVAL(&uc, 1) || (CVAL(&uc, 0)&0x80)) return;

	iconv(cd, &p, &inlen, &q, &outlen);

	if (buf[0] != CVAL(&uc, 0)) {
		printf("ucs2 char %04x not C compatible\n",
		       uc);
		error_count++;
	}
}

/* check if a character is strchr compatible */
static void check_strchr_compat(iconv_t cd, smb_ucs2_t uc)
{
	char buf[10];
	int inlen=2, outlen=10;
	int i;
	char *p = (char *)&uc;
	char *q = buf;

	iconv(cd, &p, &inlen, &q, &outlen);

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
	iconv_t cd;

	cd = iconv_open(argv[1], "UCS2");

	if (cd == (iconv_t)-1) {
		perror(argv[1]);
		return -1;
	}

	iconv(cd, NULL, NULL, NULL, NULL);
	for (i=1;i<0x10000;i++) {
		check_null(cd, i);
		check_case(cd, i);
		check_compat(cd, i);
		/* check_strchr_compat(cd, i); */
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
