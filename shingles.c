/*
  this is an attempted implementation of Broders shingles algorithm for similarity testing of documents

  see http://gatekeeper.dec.com/pub/DEC/SRC/publications/broder/positano-final-wpnums.pdf

  tridge@samba.org
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
#include <time.h>

#define MAX_WORD_SIZE 8
#define W_SHINGLES 8
#define SKETCH_SIZE 32

#define HASH_PRIME 0x01000193
#define HASH_INIT 0x28021967

#define FLAG_IGNORE_HEADERS 1

#define ROLLING_WINDOW 20

typedef unsigned u32;
typedef unsigned char uchar;

#define QSORT_CAST (int (*)(const void *, const void *))


struct u32_set {
	int count;
	u32 v[SKETCH_SIZE*2];
};


static void parse_sketch(const uchar *str, struct u32_set *v)
{
	int i, j;
	v->count = 0;
	for (i=0;i<SKETCH_SIZE;i++) {
		v->v[i] = 0;
		if (! *str) break;
		for (j=0;j<4;j++) {
			v->v[i] = (v->v[i] << 7) | *str++;
		}
		v->count++;
	}
}

static int u32_comp(u32 *v1, u32 *v2)
{
	if (*v1 > *v2) {
		return 1;
	}
	if (*v1 < *v2) {
		return -1;
	}
	return 0;
}

static void u32_union(struct u32_set *v1, struct u32_set *v2, struct u32_set *u)
{
	int i, n;
	u32 vv[SKETCH_SIZE*2];

	n = v1->count + v2->count;
	memcpy(vv, v1->v, v1->count * sizeof(u32));
	memcpy(&vv[v1->count], v2->v, v2->count * sizeof(u32));

	qsort(vv, n, sizeof(u32), QSORT_CAST u32_comp);
	
	u->count=1;
	u->v[0] = vv[0];

	for (i=1;i<n;i++) {
		if (vv[i] != u->v[u->count-1]) {
			u->v[u->count++] = vv[i];
		}
	}
}

static void u32_intersect(struct u32_set *v1, struct u32_set *v2, struct u32_set *u)
{
	u32 vv[SKETCH_SIZE*2];
	int i;

	memcpy(vv, v1->v, v1->count*sizeof(u32));
	memcpy(&vv[v1->count], v2->v, v2->count*sizeof(u32));
	qsort(vv, v1->count + v2->count, sizeof(u32), QSORT_CAST u32_comp);

	u->count=0;

	for (i=1;i<v1->count + v2->count; i++) {
		if (vv[i] == vv[i-1]) {
			u->v[u->count++] = vv[i];
		}
	}
}

static void u32_min(struct u32_set *v)
{
	qsort(v->v, v->count, sizeof(u32), QSORT_CAST u32_comp);
	if (v->count > SKETCH_SIZE) {
		v->count = SKETCH_SIZE;
	}
}

/*
  given two sketch strings return a value indicating the degree to which they match.
*/
static u32 sketch_match(const char *str1, const char *str2)
{
	u32 score = 0;
	struct u32_set v1, v2;
	struct u32_set u1;
	struct u32_set v3, v4;

	parse_sketch(str1, &v1);
	parse_sketch(str2, &v2);

	u32_union(&v1, &v2, &u1);

	u32_min(&u1);
	u32_intersect(&u1, &v1, &v3);
	u32_intersect(&v3, &v2, &v4);

	score = (100 * v4.count) / u1.count;

	return score;
}

/*
  return the maximum match for a file containing a list of sketchs
*/
static u32 sketch_match_db(const char *fname, const char *sum, u32 threshold)
{
	FILE *f;
	char line[200];
	u32 best = 0;

	f = fopen(fname, "r");
	if (!f) return 0;

	/* on each line of the database we compute the sketch match
	   score. We then pick the best score */
	while (fgets(line, sizeof(line)-1, f)) {
		u32 score;
		int len;
		len = strlen(line);
		if (line[len-1] == '\n') line[len-1] = 0;

		score = sketch_match(sum, line);

		if (score > best) {
			best = score;
			if (best >= threshold) break;
		}
	}

	fclose(f);

	return best;
}


/* a simple non-rolling hash, based on the FNV hash */
static inline u32 sum_hash(uchar c, u32 h)
{
	h *= HASH_PRIME;
	h ^= c;
	return h;
}

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

static uchar *advance(uchar *in, unsigned w, uchar *limit)
{
	in++;
	while (w--) {
		int i;
		for (i=0;i<MAX_WORD_SIZE;i++) {
			roll_hash(in[i]);
			if (in >= limit || !isalnum(in[i])) break;
		}
		in += i;
		while (in < limit && !isalnum(in[0])) {
			in++;
			roll_hash(in[i]);
		}
	}
	return in;
}

static u32 hash_emit(uchar *start, size_t len)
{
	u32 v = HASH_INIT;

	if (len == 0) return 0;

	while (len--) {
		if (isalnum(*start)) {
			v = sum_hash(*start, v);
		}
		start++;
	}
	return v;
}

struct el {
	u32 hash;
	int i;
};

static int el_comp(struct el *v1, struct el *v2)
{
	if (v2->hash > v1->hash) {
		return 1;
	}
	if (v2->hash < v1->hash) {
		return -1;
	}
	return 0;
}

static int el_comp2(struct el *v1, struct el *v2)
{
	return v1->i - v2->i;
}

static uchar *shingle_sketch(uchar *in_0, size_t size, u32 flags)
{
	uchar *in = in_0;
	uchar *p;
	struct el *sketch, *sketch2;
	uchar *ret;
	u32 sketch_size = 0;
	int i, n;
	const char *b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	if (size == 0) {
		return strdup("NULL");
	}

	/* if we are ignoring email headers then skip past them now */
	if (flags & FLAG_IGNORE_HEADERS) {
		uchar *s = strstr(in, "\n\n");
		if (s) {
			size -= (s+2 - in);
			in = in_0 = s+2;
		}
	}

	/* allocate worst case size */
	sketch  = calloc(sizeof(struct el), SKETCH_SIZE + size+1);
	sketch2 = calloc(sizeof(struct el), SKETCH_SIZE + size+1);
	ret = calloc(SKETCH_SIZE+1, 4);

	roll_reset();

	p = advance(in, W_SHINGLES, in_0 + size);

	sketch[sketch_size].hash = hash_emit(in, p-in);
	sketch[sketch_size].i = sketch_size++;

	while (p < in_0 + size) {
		in = advance(in, 1, in_0 + size);
		p = advance(p, 1, in_0 + size);
		sketch[sketch_size].hash = hash_emit(in, p-in);
		sketch[sketch_size].i = sketch_size++;
	}

	qsort(sketch, sketch_size, sizeof(struct el), QSORT_CAST el_comp);

	sketch2[0] = sketch[0];
	n = 1;
	for (i=1;i<sketch_size;i++) {
		if (sketch[i].hash != sketch2[n-1].hash) {
			sketch2[n++] = sketch[i];
		}
	}
	free(sketch);
	sketch = sketch2;
	sketch_size = n;

	if (sketch_size > SKETCH_SIZE) {
		qsort(sketch, SKETCH_SIZE, sizeof(struct el), QSORT_CAST el_comp2);
	}
	
	for (i=0;i<SKETCH_SIZE && i < sketch_size;i++) {
		u32 v = sketch[i].hash;
		int j;
		for (j=0;j<4;j++) {
			ret[4*i + j] = b64[v % 64];
			v >>= 6;
		}
	}
	ret[4*i] = 0;

	free(sketch);

	return ret;
}

/*
  return the shingles sketch on stdin
*/
static char *sketch_stdin(u32 flags)
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
	
	sum = shingle_sketch(msg, length, flags);
	
	free(msg);

	return sum;
}


/*
  return the sketch on a file
*/
char *sketch_file(const char *fname, u32 flags)
{
	int fd;
	char *sum;
	struct stat st;
	uchar *msg;

	if (strcmp(fname, "-") == 0) {
		return sketch_stdin(flags);
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

	sum = shingle_sketch(msg, st.st_size, flags);

	munmap(msg, st.st_size);

	return sum;
}


static void show_help(void)
{
	printf("
shingles v1.0 written by Andrew Tridgell <tridge@samba.org>

Syntax:
   shingles [options] <files> 
or
   shingles [options] -d sigs.txt -c SIG
or
   shingles [options] -d sigs.txt -C file

When called with a list of filenames shingles will write out the
signatures of each file on a separate line. You can specify the
filename '-' for standard input.

When called with the second form, shingles will print the best score
for the given signature with the signatures in the given database. A
score of 100 means a perfect match, and a score of 0 means a complete
mismatch.

When checking, shingles returns 0 (success) when the message *is* spam,
1 for internal errors, and 2 for messages whose signature is not
found.

The 3rd form is just like the second form, but you pass a file
containing a message instead of a pre-computed signature.

Options:
   -W              ignore whitespace
   -H              skip past mail headers
   -T <threshold>  set the threshold above which shingles will stop
                   looking (default 90)
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
	u32 threshold = 90;

	while ((c = getopt(argc, argv, "B:WHd:c:C:hT:")) != -1) {
		switch (c) {
		case 'H':
			flags |= FLAG_IGNORE_HEADERS;
			break;

		case 'd':
			dbname = optarg;
			break;

		case 'T':
			threshold = atoi(optarg);
			break;

		case 'c':
			if (!dbname) {
				show_help();
				exit(1);
			}
			score = sketch_match_db(dbname, optarg, 
						 threshold);
			printf("%u\n", score);
			exit(score >= threshold ? 0 : 2);

		case 'C':
			if (!dbname) {
				show_help();
				exit(1);
			}
			score = sketch_match_db(dbname, 
						 sketch_file(optarg, flags), 
						 threshold);
			printf("%u\n", score);
			exit(score >= threshold ? 0 : 2);

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

	/* compute the sketch on a list of files */
	for (i=0;i<argc;i++) {
		sum = sketch_file(argv[i], flags);
		printf("%s\n", sum);
		free(sum);
	}

	return 0;
}
