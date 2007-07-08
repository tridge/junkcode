/*
  LSA setsecret crypt-challenge

  This is a challenge to solve the encryption/decryption of secrets on
  a ncacn_ip_tcp LSA pipe (thats a LSA pipe on a raw TCP
  connection). We know how to decrypt when the same call is made on a
  ncacn_np pipe (thats a pipe on a SMB transport), but the same
  decryption method on the ncacn_ip_tcp pipe doesn't work. 

  have fun!

  if you solve this, please email tridge@samba.org
 */

#include "includes.h"

/*
  this is the full decrypt function that is used for this data when on
  the ncacn_np pipe. I strongly suspect its the same function when the
  ncacn_ip_tcp pipe is used, but I can't be certain. I do know that
  the decryption function used has very similar properties (such as
  encrypting in blocks of 8 bytes). What I suspect is that the session
  key is different.
 */
static void orig_decrypt(uint8_t *out, const uint8_t *in, int in_len, 
			 const uint8_t *session_key)
{
	int i, k;

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

	for (i=0,k=0;i<in_len;i += 8, k += 7) {
		uint8_t bin[8], bout[8], key[7];

		memset(bin, 0, 8);
		memcpy(bin,  &in[i], MIN(8, in_len-i));

		if (k + 7 > 16) {
			k = (16 - k);
		}
		memcpy(key, &session_key[k], 7);

		des_crypt56(bout, bin, key, 0);

		memcpy(&out[i], bout, MIN(8, in_len - i));
	}
}

#if 1
/* these are the values from a ncacn_ip_tcp pipe */
static const uint32 skey[4] = {0xd3d1432c, 0xd99698c5, 0x17fa380e, 0xb0482efd};
static const uint32 edata[8] = {0xde5ee2ad, 0x6aa713aa, 0x10322af2, 0x47a7ca3f, 
				0x5dfd3abf, 0x00f4cf88, 0x5726dd35, 0xe4c16662};
/* The plain text for the above edata is this:
   0x00000014, 0x00000001, "abcd ef12 3456 99qw erty", 0x00000000
*/



/* here is another example with a different session key */
static const uint32 skey2[4] = {0xfc4837b1, 0xc9610c90, 0xb05c0c54, 0xba68ae2b};
static const uint32 edata2[8] = {0xde5ee2ad, 0x6aa713aa, 0x10322af2, 0x47a7ca3f, 
				 0x5dfd3abf, 0x00f4cf88, 0x49fdf688, 0xaa1a3875};

/* here is an example with the same session key (skey2) but with the
   plain text differing by only 1 byte. The 'y' in the text is
   replaced with a 'z'. Notice that only the last 8 bytes change?
   Thats why I think that the algorithm is the same between the two
   transports, and only the session key differs. */
static const uint32 edata3[8] = {0xde5ee2ad, 0x6aa713aa, 0x10322af2, 0x47a7ca3f, 
				 0x5dfd3abf, 0x00f4cf88, 0x5726dd35, 0xe4c16662};

#else
/* these are values from ncacn_np - the solution to this one is just DES */
static const uint32 skey[4] = {0x71918080, 0xa98dd232, 0xbef325f8, 0xdb413b3a};
static const uint32 edata[8] = {0x17fdda34, 0xaa6ba224, 0xca4ae8d5, 0x98cb5720, 
				0x0bcedd74, 0x048356d5, 0x4ebfd5b1, 0xc0ef0607};
#endif

static void *map_file(char *fname, size_t *size)
{
	int fd = open(fname, O_RDONLY);
	struct stat st;
	void *p;
	
	fstat(fd, &st);
	p = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);

	*size = st.st_size;
	return p;
}

int main(int argc, char *argv[])
{
	uint8_t data[32];
	uint32 *x;
	unsigned char *p;
	size_t size, i, j;

	p = map_file(argv[1], &size);	

	for (i=6927476;i<size-7;i++) {
		des_crypt56(data, edata, p+i, 0);

		x = (uint32 *)data;

		if (x[0] == 20 && x[1] == 1) {
			printf("\n\nRight! i=%d\n", i);
			for (j=0;j<7;j++) {
				printf("%02x ", p[i+j]);
			}
			printf("\n");
			return 0;
		}
		if (i % 100000 == 0) {
			printf("%d\r", i);
			fflush(stdout);
		}
	}

	return 0;
}
