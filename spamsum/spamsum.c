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

/* the output is a string of length 64 in base64 */
#define SPAMSUM_LENGTH 64

#define MIN_BLOCKSIZE 3
#define HASH_PRIME 0x01000193
#define HASH_INIT 0x28021967

#define ROLLING_WINDOW 7

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

typedef unsigned u32;
typedef unsigned char uchar;

#define FLAG_IGNORE_WHITESPACE 1
#define FLAG_IGNORE_HEADERS 2

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

/*
  reset the state of the rolling hash and return the initial rolling hash value
*/
static u32 roll_reset(void)
{	
	memset(&roll_state, 0, sizeof(roll_state));
	return 0;
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
char *spamsum(const uchar *in, size_t length, u32 flags, u32 bsize)
{
	const char *b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	char *ret, *p;
	u32 total_chars;
	u32 h, h2, h3;
	u32 j, n, i, k;
	u32 block_size;
	uchar ret2[SPAMSUM_LENGTH/2 + 1];

	/* if we are ignoring email headers then skip past them now */
	if (flags & FLAG_IGNORE_HEADERS) {
		const uchar *s = strstr(in, "\n\n");
		if (s) {
			length -= (s+2 - in);
			in = s+2;
		}
	}

	if (flags & FLAG_IGNORE_WHITESPACE) {
		/* count the non-ignored chars */
		for (n=0, i=0; i<length; i++) {
			if (isspace(in[i])) continue;
			n++;
		}
		total_chars = n;
	} else {
		total_chars = length;
	}

	if (bsize == 0) {
	/* guess a reasonable block size */
		block_size = MIN_BLOCKSIZE;
		while (block_size * SPAMSUM_LENGTH < total_chars) {
			block_size = block_size * 2;
		}
	} else {
		block_size = bsize;
	}

	ret = malloc(SPAMSUM_LENGTH + SPAMSUM_LENGTH/2 + 20);
	if (!ret) return NULL;

again:
	/* the first part of the spamsum signature is the blocksize */
	snprintf(ret, 12, "%u:", block_size);
	p = ret + strlen(ret);

	memset(p, 0, SPAMSUM_LENGTH+1);
	memset(ret2, 0, sizeof(ret2));

	k = j = 0;
	h3 = h2 = HASH_INIT;
	h = roll_reset();

	for (i=0; i<length; i++) {
		if ((flags & FLAG_IGNORE_WHITESPACE) && 
		    isspace(in[i])) continue;

		/* 
		   at each character we update the rolling hash and
		   the normal hash. When the rolling hash hits the
		   reset value then we emit the normal hash as a
		   element of the signature and reset both hashes
		*/
		h = roll_hash(in[i]);
		h2 = sum_hash(in[i], h2);
		h3 = sum_hash(in[i], h3);

		if (h % block_size == (block_size-1)) {
			/* we have hit a reset point. We now emit a
			   hash which is based on all chacaters in the
			   piece of the message between the last reset
			   point and this one */
			p[j] = b64[h2 % 64];
			if (j < SPAMSUM_LENGTH-1) {
				/* we can have a problem with the tail
				   overflowing. The easiest way to
				   cope with this is to only reset the
				   second hash if we have room for
				   more characters in our
				   signature. This has the effect of
				   combining the last few pieces of
				   the message into a single piece */
				h2 = HASH_INIT;
				j++;
			}
		}

		/* this produces a second signature with a block size
		   of block_size*2. By producing dual signatures in
		   this way the effect of small changes in the message
		   size near a block size boundary is greatly reduced. */
		if (h % (block_size*2) == ((block_size*2)-1)) {
			ret2[k] = b64[h3 % 64];
			if (k < SPAMSUM_LENGTH/2-1) {
				h3 = HASH_INIT;
				k++;
			}
		}
	}

	/* if we have anything left then add it to the end. This
	   ensures that the last part of the message is always
	   considered */
	if (h != 0) {
		p[j] = b64[h2 % 64];
		ret2[k] = b64[h3 % 64];
	}

	p[++j] = ':';
	strcat(p+j, ret2);

	/* our blocksize guess may have been way off - repeat if necessary */
	if (bsize == 0 && block_size > MIN_BLOCKSIZE && j < SPAMSUM_LENGTH/2) {
		block_size = block_size / 2;
		goto again;
	}

	return ret;
}


/* 
   we only accept a match if we have at least one common substring in
   the signature of length ROLLING_WINDOW. This dramatically drops the
   false positive rate for low score thresholds while having
   negligable affect on the rate of spam detection.

   return 1 if the two strings do have a common substring, 0 otherwise
*/
static int has_common_substring(const char *s1, const char *s2)
{
	int i, j;
	int num_hashes;
	u32 hashes[SPAMSUM_LENGTH];

	/* there are many possible algorithms for common substring
	   detection. In this case I am re-using the rolling hash code
	   to act as a filter for possible substring matches */

	roll_reset();
	memset(hashes, 0, sizeof(hashes));

	/* first compute the windowed rolling hash at each offset in
	   the first string */
	for (i=0;s1[i];i++) {
		hashes[i] = roll_hash((uchar)s1[i]);
	}
	num_hashes = i;

	roll_reset();

	/* now for each offset in the second string compute the
	   rolling hash and compare it to all of the rolling hashes
	   for the first string. If one matches then we have a
	   candidate substring match. We then confirm that match with
	   a direct string comparison */
	for (i=0;s2[i];i++) {
		u32 h = roll_hash((uchar)s2[i]);
		for (j=0;j<num_hashes;j++) {
			if (hashes[j] != 0 && hashes[j] == h) {
				/* we have a potential match - confirm it */
				if (strlen(s2+i) >= ROLLING_WINDOW && 
				    strncmp(s2+i, s1+j, ROLLING_WINDOW) == 0) {
					return 1;
				}
			}
		}
	}

	return 0;
}


/*
  eliminate sequences of longer than 3 identical characters. These
  sequences contain very little information so they tend to just bias
  the result unfairly
*/
static char *eliminate_sequences(const char *str)
{
	char *ret;
	int i, j, len;

	ret = strdup(str);
	if (!ret) return NULL;

	len = strlen(str);

	for (i=j=3;i<len;i++) {
		if (str[i] != str[i-1] ||
		    str[i] != str[i-2] ||
		    str[i] != str[i-3]) {
			ret[j++] = str[i];
		}
	}

	ret[j] = 0;

	return ret;
}

/*
  this is the low level string scoring algorithm. It takes two strings
  and scores them on a scale of 0-100 where 0 is a terrible match and
  100 is a great match. The block_size is used to cope with very small
  messages.
*/
static unsigned score_strings(const char *s1, const char *s2, u32 block_size)
{
	u32 score;
	u32 len1, len2;
	int edit_distn(const char *from, int from_len, const char *to, int to_len);

	len1 = strlen(s1);
	len2 = strlen(s2);

	if (len1 > SPAMSUM_LENGTH || len2 > SPAMSUM_LENGTH) {
		/* not a real spamsum signature? */
		return 0;
	}

	/* the two strings must have a common substring of length
	   ROLLING_WINDOW to be candidates */
	if (has_common_substring(s1, s2) == 0) {
		return 0;
	}

	/* compute the edit distance between the two strings. The edit distance gives
	   us a pretty good idea of how closely related the two strings are */
	score = edit_distn(s1, len1, s2, len2);

	/* scale the edit distance by the lengths of the two
	   strings. This changes the score to be a measure of the
	   proportion of the message that has changed rather than an
	   absolute quantity. It also copes with the variability of
	   the string lengths. */
	score = (score * SPAMSUM_LENGTH) / (len1 + len2);

	/* at this stage the score occurs roughly on a 0-64 scale,
	 * with 0 being a good match and 64 being a complete
	 * mismatch */

	/* rescale to a 0-100 scale (friendlier to humans) */
	score = (100 * score) / 64;

	/* it is possible to get a score above 100 here, but it is a
	   really terrible match */
	if (score >= 100) return 0;

	/* now re-scale on a 0-100 scale with 0 being a poor match and
	   100 being a excellent match. */
	score = 100 - score;

	/* when the blocksize is small we don't want to exaggerate the match size */
	if (score > block_size/MIN_BLOCKSIZE * MIN(len1, len2)) {
		score = block_size/MIN_BLOCKSIZE * MIN(len1, len2);
	}

	return score;
}

/*
  given two spamsum strings return a value indicating the degree to which they match.
*/
u32 spamsum_match(const char *str1, const char *str2)
{
	u32 block_size1, block_size2;
	u32 score = 0;
	char *s1, *s2;
	char *s1_1, *s1_2;
	char *s2_1, *s2_2;

	/* each spamsum is prefixed by its block size */
	if (sscanf(str1, "%u:", &block_size1) != 1 ||
	    sscanf(str2, "%u:", &block_size2) != 1) {
		return 0;
	}

	/* if the blocksizes don't match then we are comparing
	   apples to oranges ... */
	if (block_size1 != block_size2 && 
	    block_size1 != block_size2*2 &&
	    block_size2 != block_size1*2) {
		return 0;
	}

	/* move past the prefix */
	str1 = strchr(str1, ':');
	str2 = strchr(str2, ':');

	if (!str1 || !str2) {
		/* badly formed ... */
		return 0;
	}

	/* there is very little information content is sequences of
	   the same character like 'LLLLL'. Eliminate any sequences
	   longer than 3. This is especially important when combined
	   with the has_common_substring() test below. */
	s1 = eliminate_sequences(str1+1);
	s2 = eliminate_sequences(str2+1);

	if (!s1 || !s2) return 0;

	/* now break them into the two pieces */
	s1_1 = s1;
	s2_1 = s2;

	s1_2 = strchr(s1, ':');
	s2_2 = strchr(s2, ':');

	if (!s1_2 || !s2_2) {
		/* a signature is malformed - it doesn't have 2 parts */
		free(s1); free(s2);
		return 0;
	}

	*s1_2++ = 0;
	*s2_2++ = 0;

	/* each signature has a string for two block sizes. We now
	   choose how to combine the two block sizes. We checked above
	   that they have at least one block size in common */
	if (block_size1 == block_size2) {
		u32 score1, score2;
		score1 = score_strings(s1_1, s2_1, block_size1);
		score2 = score_strings(s1_2, s2_2, block_size2);
		score = MAX(score1, score2);
	} else if (block_size1 == block_size2*2) {
		score = score_strings(s1_1, s2_2, block_size1);
	} else {
		score = score_strings(s1_2, s2_1, block_size2);
	}

	free(s1);
	free(s2);

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

	/* on each line of the database we compute the spamsum match
	   score. We then pick the best score */
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
static char *spamsum_stdin(u32 flags, u32 block_size)
{
	uchar buf[10*1024];
	uchar *msg;
	size_t length = 0;
	int n;
	char *sum;

	msg = malloc(sizeof(buf));
	if (!msg) return NULL;

	/* load the file, expanding the allocation as needed. */
	while (1) {
		n = read(0, buf, sizeof(buf));
		if (n == -1 && errno == EINTR) continue;
		if (n <= 0) break;

		msg = realloc(msg, length + n);
		if (!msg) return NULL;

		memcpy(msg+length, buf, n);
		length += n;
	}
	
	sum = spamsum(msg, length, flags, block_size);
	
	free(msg);

	return sum;
}


/*
  return the spamsum on a file
*/
char *spamsum_file(const char *fname, u32 flags, u32 block_size)
{
	int fd;
	char *sum;
	struct stat st;
	uchar *msg;

	if (strcmp(fname, "-") == 0) {
		return spamsum_stdin(flags, block_size);
	} 

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

	sum = spamsum(msg, st.st_size, flags, block_size);

	munmap(msg, st.st_size);

	return sum;
}

static void show_help(void)
{
	printf("
spamsum v1.1 written by Andrew Tridgell <tridge@samba.org>

spamsum computes a signature string that is particular good for detecting if two emails
are very similar. This can be used to detect SPAM.

Syntax:
   spamsum [options] <files> 
or
   spamsum [options] -d sigs.txt -c SIG
or
   spamsum [options] -d sigs.txt -C file

When called with a list of filenames spamsum will write out the signatures of each file
on a separate line. You can specify the filename '-' for standard input.

When called with the second form, spamsum will print the best score
for the given signature with the signatures in the given database. A
score of 100 means a perfect match, and a score of 0 means a complete
mismatch.

The 3rd form is just like the second form, but you pass a file
containing a message instead of a pre-computed signature.

Options:
   -W              ignore whitespace
   -H              skip past mail headers
   -B <bsize>      force a block size of bsize
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
	u32 flags = 0;
	u32 block_size = 0;

	while ((c = getopt(argc, argv, "B:WHd:c:C:h")) != -1) {
		switch (c) {
		case 'W':
			flags |= FLAG_IGNORE_WHITESPACE;
			break;

		case 'H':
			flags |= FLAG_IGNORE_HEADERS;
			break;

		case 'd':
			dbname = optarg;
			break;

		case 'B':
			block_size = atoi(optarg);
			break;

		case 'c':
			if (!dbname) {
				show_help();
				exit(1);
			}
			score = spamsum_match_db(dbname, optarg);
			printf("%u\n", score);
			exit(0);

		case 'C':
			if (!dbname) {
				show_help();
				exit(1);
			}
			score = spamsum_match_db(dbname, 
						 spamsum_file(optarg, flags, 
							      block_size));
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
		sum = spamsum_file(argv[i], flags, block_size);
		printf("%s\n", sum);
		free(sum);
	}

	return 0;
}
