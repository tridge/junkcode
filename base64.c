#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

/*
  encode as base64
  caller frees
*/
static char *base64_encode(const char *buf, int len)
{
	const char *b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int bit_offset, byte_offset, idx, i;
	const unsigned char *d = (const unsigned char *)buf;
	int bytes = (len*8 + 5)/6, pad_bytes = (bytes % 4) ? 4 - (bytes % 4) : 0;
	char *out;

	out = calloc(1, bytes+pad_bytes+1);
	if (!out) return NULL;

	for (i=0;i<bytes;i++) {
		byte_offset = (i*6)/8;
		bit_offset = (i*6)%8;
		if (bit_offset < 3) {
			idx = (d[byte_offset] >> (2-bit_offset)) & 0x3F;
		} else {
			idx = (d[byte_offset] << (bit_offset-2)) & 0x3F;
			if (byte_offset+1 < len) {
				idx |= (d[byte_offset+1] >> (8-(bit_offset-2)));
			}
		}
		out[i] = b64[idx];
	}

	for (;i<bytes+pad_bytes;i++)
		out[i] = '=';
	out[i] = 0;

	return out;
}


static int load_stdin(char **s)
{
	char buf[1024];
	int length = 0;

	*s = NULL;

	while (1) {
		int n = read(0, buf, sizeof(buf));
		if (n == -1 && errno == EINTR) continue;
		if (n <= 0) break;

		(*s) = realloc((*s), length + n + 1);
		if (!(*s)) return 0;

		memcpy((*s)+length, buf, n);
		length += n;
	}

	(*s)[length] = 0;
	
	return length;
}

int main(int argc, char *argv[])
{
	char *s;
	int n;

	n = load_stdin(&s);

	printf("%s\n", base64_encode(s, n));
	return 0;
}
