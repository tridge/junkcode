#include <stdio.h>
#include <string.h>


/***************************************************************************
decode a base64 string in-place - simple and slow algorithm
  ***************************************************************************/
static int base64_decode(char *s)
{
	char *b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int bit_offset, byte_offset, idx, i, n;
	unsigned char *d = (unsigned char *)s;
	char *p;

	n=i=0;

	while (*s && (p=strchr(b64,*s))) {
		idx = (int)(p - b64);
		byte_offset = (i*6)/8;
		bit_offset = (i*6)%8;
		d[byte_offset] &= ~((1<<(8-bit_offset))-1);
		if (bit_offset < 3) {
			d[byte_offset] |= (idx << (2-bit_offset));
			n = byte_offset+1;
		} else {
			d[byte_offset] |= (idx >> (bit_offset-2));
			d[byte_offset+1] = 0;
			d[byte_offset+1] |= (idx << (8-(bit_offset-2))) & 0xFF;
			n = byte_offset+2;
		}
		s++; i++;
	}
	/* null terminate */
	d[n] = 0;
	return n;
}

static void usage(void)
{
	printf("
Usage: ubase64 <string>
");
}

int main(int argc, char *argv[])
{
	char *s;
	int n;

	if (argc < 2) {
		usage();
		exit(1);
	}
	
	s = strdup(argv[1]);

	n = base64_decode(s);
	write(1, s, n);
	return 0;
}
