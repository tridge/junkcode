/*
  this is a checksum routine that is specifically designed for spam. 
  Copyright Andrew Tridgell <tridge@samba.org> 2002

  This code is released under the GNU General Public License version 2 or later.

  If you wish to distribute this code under the terms of a different
  free software license then please ask me. If there is a good reason
  then I will probably say yes.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

/* we ignore all spaces */
#define ignore_char(c) isspace(c)

/* the output is a string of length 64 in base64 */
#define SPAMSUM_LENGTH 64

#define MIN_BLOCKSIZE 3
#define HASH_PRIME 0x01000193
#define HASH_INIT 0x28021967

#define ROLLING_WINDOW 7

typedef unsigned u32;
typedef unsigned char uchar;

static struct {
	uchar window[ROLLING_WINDOW];
	u32 h1, h2, h3;
	u32 n;
} roll_state;

/*
  a rolling hash, based on the Adler checksum. By using a rolling hash
  we can perform auto resynchronisation after inserts/deletes

  internally, h1 is the sum of the bytes in the window and h2
  is the sum of the bytes times the index

  h3 is a shift/xor based rolling hash, and is mostly needed to ensure that
  we can cope with large blocksize values
*/
static inline u32 roll_hash(uchar c)
{
	roll_state.h2 -= roll_state.h1;
	roll_state.h2 += ROLLING_WINDOW * c;

	roll_state.h1 += c;
	roll_state.h1 -= roll_state.window[roll_state.n % ROLLING_WINDOW];

	roll_state.window[roll_state.n % ROLLING_WINDOW] = c;
	roll_state.n++;

	roll_state.h3 = (roll_state.h3 << 5) & 0xFFFFFFFF;
	roll_state.h3 ^= c;

	return roll_state.h1 + roll_state.h2 + roll_state.h3;
}

/* a simple non-rolling hash, based on the FNV hash */
static inline u32 sum_hash(uchar c, u32 h)
{
	h *= HASH_PRIME;
	h ^= c;
	return h;
}

/*
  take a message of length 'length' and return a string representing a hash of that message,
  prefixed by the selected blocksize
*/
char *spamsum(const uchar *in, size_t length)
{
	const char *b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	char *ret, *p;
	u32 total_chars;
	u32 h, h2;
	u32 block_size, j, n, i;

	/* count the non-ignored chars */
	for (n=0, i=0; i<length; i++) {
		if (ignore_char(in[i])) continue;
		n++;
	}
	total_chars = n;

	/* guess a reasonable block size */
	block_size = MIN_BLOCKSIZE;
	while (block_size * SPAMSUM_LENGTH < total_chars) {
		block_size = block_size * 2;
	}

	ret = malloc(SPAMSUM_LENGTH + 20);
	if (!ret) return NULL;

again:
	/* the first part of the spamsum signature is the blocksize */
	snprintf(ret, 12, "%u:", block_size);
	p = ret + strlen(ret);

	memset(p, 0, SPAMSUM_LENGTH+1);

	j = 0;
	h2 = HASH_INIT;
	h = 0;
	memset(&roll_state, 0, sizeof(roll_state));

	for (i=0; i<length; i++) {
		if (ignore_char(in[i])) continue;

		/* 
		   at each character we update the rolling hash and the normal hash. When the rolling
		   hash hits the reset value then we emit the normal hash as a element of the signature
		   and reset both hashes
		*/
		h = roll_hash(in[i]);
		h2 = sum_hash(in[i], h2);

		if (h % block_size == (block_size-1)) {
			p[j] = b64[h2 % 64];
			memset(&roll_state, 0, sizeof(roll_state));
			h = 0;
			if (j < SPAMSUM_LENGTH-1) {
				h2 = HASH_INIT;
				j++;
			}
		}
	}

	/* our blocksize guess may have been way off - repeat if necessary */
	if (block_size > MIN_BLOCKSIZE && j < SPAMSUM_LENGTH/2) {
		block_size = block_size / 2;
		goto again;
	}

	return ret;
}


/*
  given two spamsum strings return a value indicating the degree to which they match.
*/
u32 spamsum_match(const char *s1, const char *s2)
{
	u32 block_size1, block_size2;
	u32 len1, len2;
	u32 score = 0;
	int edit_distn(const char *from, int from_len, const char *to, int to_len);

	if (sscanf(s1, "%u:", &block_size1) != 1 ||
	    sscanf(s2, "%u:", &block_size2) != 1 ||
	    block_size1 != block_size2) {
		/* if the blocksizes don't match then we are comparing
		   apples to oranges ... */
		return 0;
	}

	s1 = strchr(s1, ':') + 1;
	s2 = strchr(s2, ':') + 1;

	len1 = strlen(s1);
	len2 = strlen(s2);

	/* compute the edit distance between the two strings. The edit distance gives
	   us a pretty good idea of how closely related the two strings are */
	score = edit_distn(s1, len1, s2, len2);
	if (score >= 100) return 0;

	/* now re-scale on a 0-100 scale, using a square to weight
	   towards high degrees of matches */
	score = 100 - score;
	score = score * score / 100;

	/* when the blocksize is small we don't want to exaggerate the match size */
	if (score > block_size1/MIN_BLOCKSIZE * len1) {
		score = block_size1/MIN_BLOCKSIZE * len1;
	}
	if (score > block_size1/MIN_BLOCKSIZE * len2) {
		score = block_size1/MIN_BLOCKSIZE * len2;
	}

	return score;
}

/*
  return the maximum match for a file containing a list of spamsums
*/
u32 spamsum_match_db(const char *fname, const char *sum)
{
	FILE *f;
	char line[100];
	u32 best = 0;

	f = fopen(fname, "r");
	if (!f) return 0;

	while (fgets(line, sizeof(line)-1, f)) {
		u32 score;
		int len;
		len = strlen(line);
		if (line[len-1] == '\n') line[len-1] = 0;

		score = spamsum_match(sum, line);

		if (score > best) best = score;
	}

	fclose(f);

	return best;
}

/*
  return the spamsum on stdin
*/
char *spamsum_stdin(void)
{
	uchar buf[10*1024];
	uchar *msg;
	size_t length = 0;
	int n;
	char *sum;

	msg = malloc(sizeof(buf));
	if (!msg) return NULL;

	while (1) {
		n = read(0, buf, sizeof(buf));
		if (n == -1 && errno == EINTR) continue;
		if (n <= 0) break;

		msg = realloc(msg, length + n);
		if (!msg) return NULL;

		memcpy(msg+length, buf, n);
		length += n;
	}
	
	sum = spamsum(msg, length);
	
	free(msg);

	return sum;
}


/*
  return the spamsum on a file
*/
char *spamsum_file(const char *fname)
{
	int fd;
	char *sum;
	struct stat st;
	uchar *msg;

	fd = open(fname, O_RDONLY);
	if (fd == -1) {
		perror(fname);
		return NULL;
	}

	if (fstat(fd, &st) == -1) {
		perror("fstat");
		return NULL;
	}

	msg = mmap(NULL, st.st_size, PROT_READ, MAP_FILE|MAP_PRIVATE, fd, 0);
	if (msg == (uchar *)-1) {
		perror("mmap");
		return NULL;
	}
	close(fd);

	sum = spamsum(msg, st.st_size);

	munmap(msg, st.st_size);

	return sum;
}

static void show_help(void)
{
	printf("
spamsum v1.0 written by Andrew Tridgell <tridge@samba.org>

spamsum computes a signature string that is particular good for detecting if two emails
are very similar. This can be used to detect SPAM.

Syntax:
   spamsum <files> 
or
   spamsum -d sigs.txt -c SIG

When called with a list of filenames spamsum will write out the signatures of each file
on a separate line. You can specify the filename '-' for standard input.

When called with the second form, spamsum will print the best score
for the given signature with the signatures in the given database. A
score of 100 means a perfect match, and a score of 0 means a complete
mismatch.
");
}

int main(int argc, char *argv[])
{
	char *sum;
	extern char *optarg;
	extern int optind;
	int c;
	char *dbname = NULL;
	u32 score;
	int i;

	while ((c = getopt(argc, argv, "d:c:h")) != -1) {
		switch (c) {
		case 'd':
			dbname = optarg;
			break;

		case 'c':
			if (!dbname) {
				show_help();
				exit(1);
			}
			score = spamsum_match_db(dbname, optarg);
			printf("%u\n", score);
			exit(0);

		case 'h':
		default:
			show_help();
			exit(0);
		}
	}

	argc -= optind;
	argv += optind;

	if (argc == 0) {
		show_help();
		return 0;
	}

	/* compute the spamsum on a list of files */
	for (i=0;i<argc;i++) {
		if (strcmp(argv[i], "-") == 0) {
			sum = spamsum_stdin();
		} else {
			sum = spamsum_file(argv[i]);
		}
		printf("%s\n", sum);
		free(sum);
	}

	return 0;
}
