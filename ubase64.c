#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>


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

	while (*s && ((*s == '\n') || (p=strchr(b64,*s)))) {
		if (*s == '\n') {
			s++;
			continue;
		}
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

	n = base64_decode(s);
	write(1, s, n);
	return 0;
}
