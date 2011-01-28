#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

static const struct {
	uint8_t value;
	char c;
} digits[] = {
	{ 0x42, '0' },
	{ 0x61, '1' },
	{ 0x04, '2' },
	{ 0xE6, '3' },
	{ 0xA5, '4' },
	{ 0x2E, '5' },
	{ 0x4E, '6' },
	{ 0xE1, '7' },
	{ 0x46, '8' },
	{ 0x26, '9' },
	{ 0x67, ' ' },
	{ 0xC9, 'L' },
};


static const struct {
	uint8_t ofs1, ofs2;
	uint8_t extra;
	uint8_t adder;
} info[4] = {
	{ 10, 3, 0xF1, 0 },
	{  9, 6, 0x5F, 0 },
	{  7, 5, 0x46, 0x11 },
	{  0, 2, 0xBD, 0xFF }
};


/*
000000
00004B
177176
177186
377176
378176
379176
3791B6
37F176
577176
577186
F71176
F77176
F77196
F79176
F791B6
F791F6
F7B176

8F 6E AC == Hz
BF 6E 6C == C
8F 8E 6C == nF
8F 6E 6C == MOhm
8F AE 6C == KOhm
8F 6E 8C == mV
8F 7E 7C == uA
8F 6E 7C == mA

AC:     4B 47 1D 21 71 36 2F 54 76 55 4F 8F 6E 7C 
AC rel  4B 07 1D 21 B1 36 2F 54 76 55 4F 8F 6E 7C 
DC:     4B 57 1D 21 71 36 2F 54 76 55 4F 8F 6E 7C 
DC rel: 4B 17 1D 21 B1 36 2F 54 76 55 4F 8F 6E 7C 

data[8]:
4B
76
86
96
B6
F6

*/

int main(int argc, const char *argv[])
{
	uint8_t data[14] = { 0x00, 0x37, 0x00, 0x00, 0x71, 0x00, 0x00, 
			     0x00, 0x76, 0x00, 0x00, 0xBF, 0x6E, 0x6C };
	int i;
	FILE *f, *f2;
	char *str, *tok;

	str = strdup(argv[1]);
	for (i=0; i<4; i++, str++) {
		uint8_t v1=0, v2=0;
		bool decimal = false;
		int j;
		uint8_t dval;

		if (*str == '.' || *str == '-') {
			decimal = true;
			str++;
		}
		for (j=0; j<ARRAY_SIZE(digits); j++) {
			if (digits[j].c == *str) {
				break;
			}
		}
		if (j==ARRAY_SIZE(digits)) {
			fprintf(stderr, "Unknown character '%c'\n", *str);
			exit(1);
		}
		dval = digits[j].value;
		if (decimal) {
			dval += 0x10;
		}
		dval += info[i].adder;
		v1 |= (dval & 0xF0);
		v2 |= (dval & 0x0F) << 4;
		v1 |= (info[i].extra & 0xF0) >> 4;
		v2 |= (info[i].extra & 0x0F);
		data[info[i].ofs1] = v1;
		data[info[i].ofs2] = v2;
	}

	for (tok=strtok(str, " "); tok; tok=strtok(NULL, " ")) {
		if (strcmp(tok, "rel") == 0) {
			data[1] ^= 0x40;
			data[4] ^= 0x40;
		}
		if (strcmp(tok, "AC") == 0) {
			data[1] = 0x47;
			data[4] = 0x71;
		}
		if (strcmp(tok, "DC") == 0) {
			data[1] = 0x57;
			data[4] = 0x71;
		}
		if (strcmp(tok, "Hz") == 0) {
			data[11] = 0x8F;
			data[12] = 0x6E;
			data[13] = 0xAC;
		}
		if (strcmp(tok, "C") == 0) {
			data[1] = 0x37;
			data[4] = 0x71;
			data[11] = 0xBF;
			data[12] = 0x6E;
			data[13] = 0x6C;
		}
		if (strcmp(tok, "nF") == 0) {
			data[11] = 0x8F;
			data[12] = 0x8E;
			data[13] = 0x6C;
		}
		if (strcmp(tok, "Ohm") == 0) {
			data[1] = 0x37;
			data[4] = 0x11;
			data[8]  = 0x76;
			data[11] = 0x8F;
			data[12] = 0x6E;
			data[13] = 0x6C;
		}
		if (strcmp(tok, "kOhm") == 0) {
			data[1] = 0x37;
			data[4] = 0x11;
			data[8]  = 0x76;
			data[11] = 0x8F;
			data[12] = 0xAE;
			data[13] = 0x6C;
		}
		if (strcmp(tok, "MOhm") == 0) {
			data[1] = 0x37;
			data[4] = 0x11;
			data[8]  = 0xB6;
			data[11] = 0x8F;
			data[12] = 0x6E;
			data[13] = 0x6C;
		}
		if (strcmp(tok, "mV") == 0) {
			data[8]  = 0x86;
			data[11] = 0x8F;
			data[12] = 0x6E;
			data[13] = 0x8C;
		}
		if (strcmp(tok, "uA") == 0) {
			data[8]  = 0x76;
			data[11] = 0x8F;
			data[12] = 0x7E;
			data[13] = 0x7C;
		}
		if (strcmp(tok, "mA") == 0) {
			data[8]  = 0x86;
			data[11] = 0x8F;
			data[12] = 0x6E;
			data[13] = 0x7C;
		}
		if (strcmp(tok, "A") == 0) {
			data[8]  = 0x76;
			data[11] = 0x8F;
			data[12] = 0x6E;
			data[13] = 0x7C;
		}
		if (strcmp(tok, "mF") == 0) {
			data[1] = 0x37;
			data[4] = 0x11;
			data[8]  = 0x86;
			data[11] = 0x8F;
			data[12] = 0x6E;
			data[13] = 0x6C;
		}
	}

	f = fopen("/tmp/usb.data.tmp", "w");
	for (i=0;i<14;i++) {
		fprintf(f,"%02X ", data[i]);
	}
	fprintf(f, "\n\n");
	f2 = fopen("/tmp/usb.data", "r");
	if (f2) {
		char buf[100];
		size_t len;
		while ((len = fread(buf, 1, sizeof(buf)-1, f2)) > 0) {
			fwrite(buf, 1, len, f);
		}
		fclose(f2);
	}
	fclose(f);
	chmod("/tmp/usb.data.tmp", 0777);
	rename("/tmp/usb.data.tmp", "/tmp/usb.data");
	printf("OK\n");
	return 0;
}

