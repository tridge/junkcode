/*****************************************************************************
blockhash matching engine

Copyright Andrew Tridgell <tridge@samba.org> July 2003

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.

******************************************************************************/
#include "includes.h"


struct hash {
	uint8 v[16];
	int key;
	char *note;
	unsigned used;
};

static struct {
	int match_threshold;
	int fuzz_threshold;
	int html_format;
	int curses_format;
	int max_uses;
	int avoid_self_matches;
} options;

static struct {
	int token_width;
	int token_skip;
	int count;
	struct hash *hash;
	int *hash_index;
} hashes;

#define INDEX_BITS 20
#define INDEX_SIZE (1<<INDEX_BITS)
#define INDEX_MASK (INDEX_SIZE-1)

static int verbose;
static const char *filename_pattern = "*";

/*
  form a  integer from 4 bytes in a machine independent fashion, masked 
  by INDEX_MASK
 */
static int hash_key(const uint8 *v)
{
	uint32 h = (((uint32)v[0])<<24) | 
		(((uint32)v[1])<<16) |
		(((uint32)v[2])<<8) |
		(((uint32)v[3]));
	return h & INDEX_MASK;
}

/*
  load one hash
*/
static int load_hash(FILE *f, struct hash *hash)
{
	int i;
	char buf[1024];
	static char *last_note;

	for (i=0;i<16;i++) {
		unsigned int v;
		if (fscanf(f, "%02X", &v) != 1) {
			return -1;
		}
		hash->v[i] = v;
	}
	hash->key = hash_key(&hash->v[0]);

	fgets(buf, sizeof(buf), f);
	if (buf[strlen(buf)-1] == '\n') {
		buf[strlen(buf)-1] = 0;
	}

	if (last_note && strcmp(last_note, buf) == 0) {
		hash->note = last_note;
	} else {
		hash->note = strdup(buf);
		last_note = hash->note;
	}

	return 0;
}

/* compare two hashes (for sorting *) */
int hash_cmp(struct hash *h1, struct hash *h2) 
{
	if (h1->key == h2->key) {
		return memcmp(h1->v, h2->v, 16);
	}
	return h1->key - h2->key;
}

/* remove repeated hashes, counting how many repeats for each
   hash */
static void remove_repeats(void)
{
	int h, i = 0;
	struct hash *h2 = calloc(hashes.count, sizeof(struct hash));

	if (!h2) {
		printf("WARNING: Unable to allocate repeat array\n");
		return;
	}

	h2[i++] = hashes.hash[0];
	for (h=1;h<hashes.count;h++) {
		if (memcmp(hashes.hash[h].v, hashes.hash[h-1].v, 16) != 0) {
			h2[i++] = hashes.hash[h];
		}
	}
	printf("Removed %d repeated hashes\n", hashes.count - i);
	free(hashes.hash);
	hashes.hash = h2;
	hashes.count = i;
}


/*
  form the hash index
*/
static void form_index(void)
{
	int h, b;

	/* sort the hashes by the key */
	qsort(hashes.hash, hashes.count, sizeof(struct hash), (__compar_fn_t)hash_cmp);

	remove_repeats();

	hashes.hash_index = (int *)calloc(INDEX_SIZE, sizeof(int));

	for (h=0;h<INDEX_SIZE;h++) {
		hashes.hash_index[h] = -1;
	}

	/* the index is into the first hash with each value */
	hashes.hash_index[hashes.hash[0].key] = 0;

	for (b=1;b<hashes.count;b++) {
		if (hashes.hash[b].key != hashes.hash[b-1].key) {
			hashes.hash_index[hashes.hash[b].key] = b;
		}
	}
}

/*
  load the hashes and parameters from a hashfile
*/
static void load_hashes(const char *fname)
{
	FILE *f;
	int max_hashes, i;
	char version[11];

	/* (over) estimate the number of hashes */
	max_hashes = file_lines(fname);

	hashes.hash = (struct hash *)calloc(max_hashes, sizeof(struct hash));
	if (!hashes.hash) {
		printf("Failed to allocate hashes array\n");
		exit(1);
	}

	f = fopen(fname, "r");
	if (!f) {
		printf("Failed to open %s - %s\n", fname, strerror(errno));
		exit(1);
	}
	
	if (fscanf(f, "version %10s of blockhash\n", version) != 1 ||
	    strcmp(version, VERSION) != 0) {
		printf("Bad version tag in hashfile\n");
		exit(1);
	}

	if (fscanf(f, "token_width %d\n", &hashes.token_width) != 1 ||
	    fscanf(f, "token_skip %d\n", &hashes.token_skip) != 1) {
		printf("Failed to read token_width/token_skip tags\n");
		exit(1);
	}

	if (hashes.token_skip > hashes.token_width ||
	    hashes.token_skip <= 0) {
		printf("Bad token_width/token_skip tags\n");
		exit(1);
	}

	for (i=0; i<max_hashes; i++) {
		if (load_hash(f, &hashes.hash[i]) != 0) break;
	}
	hashes.count = i;

	printf("Loaded %d hashes\n", hashes.count);

	form_index();
}

struct match_struct {
	FILE *f;
	const char *fname;
};


/*
  register a match
*/
static void register_match(const char *fname, int line, struct hash *hash, struct char_range *ranges)
{
#define MAX_RANGES 1000
	static struct {
		char *name, *note;
		int start, end;
		int num_ranges;
		struct char_range ranges[MAX_RANGES];
	} last;

	void show_match(void) {
		const char *h_start, *h_end;
		if (options.html_format) {
			h_start = "<font color=red>";
			h_end = "</font>";
		} else if (options.curses_format) {
			h_start = "\033[31;47m";
			h_end = "\033[0m";
		} else {
			h_start = h_end = "";
		}
		if (last.end - last.start >= options.match_threshold) {
			if (options.html_format) {
				printf("<br><hr><br><font color=blue>\n");
			}
			printf("%s(%d:%d)", last.name, last.start, last.end);
			if (last.note) {
				if (options.html_format) {
					printf("</font><font color=green>\n");
				}
				printf("   %s\n", last.note);
			}
			if (options.html_format) {
				printf("</font><br><pre>\n");
			}
			dump_lines(last.name, last.start, last.end, 
				   last.num_ranges, last.ranges,
				   h_start, h_end);
			if (options.html_format) {
				printf("</pre><br>\n");
			}
		}
		free(last.name);
		free(last.note);
		memset(&last, 0, sizeof(last));
	}

	if (!fname) {
		if (last.name) {
			show_match();
		}
		return;
	}

	hash->used++;

	if (last.name && 
	    strcmp(fname, last.name) == 0 && 
	    last.num_ranges < MAX_RANGES) {
		if (last.end >= line - (1+options.fuzz_threshold)) {
			last.end = line;
			if (last.ranges[last.num_ranges-1].end >= ranges[0].start) {
				last.ranges[last.num_ranges-1].end = ranges[hashes.token_width-1].end;
			} else {
				last.ranges[last.num_ranges].start = ranges[0].start;
				last.ranges[last.num_ranges].end = ranges[hashes.token_width-1].end;
				last.num_ranges++;
			}
			return;
		}
	}

	if (hash->used > options.max_uses) {
		return;
	}

	if (last.name) {
		show_match();
	}

	last.name = strdup(fname);
	last.note = strdup(hash->note);
	last.start = line;
	last.end = line;
	last.num_ranges = 1;
	last.ranges[0].start = ranges[0].start;
	last.ranges[0].end = ranges[hashes.token_width-1].end;
}

/*
  check one hash for matches
*/
static void hash_check(const char *buf, const uint8 *v, void *private, int line, struct char_range *ranges)
{
	struct match_struct *m = private;
	int key, i;

	key = hash_key(v);
	if (hashes.hash_index[key] == -1) {
		/* definately no match */
		return;
	}

	for (i=hashes.hash_index[key]; 
	     i<hashes.count && hashes.hash[i].key == key; 
	     i++) {
		if (memcmp(v, hashes.hash[i].v, 16) == 0) {
			int b;
			const char *p;
			/* a match! */

			if (options.avoid_self_matches && 
			    hashes.hash[i].note) {
				char *p, *p2;
				p = hashes.hash[i].note;
				while (isspace(*p)) p++;
				p2 = strchr(p,'(');
				if (p2 && strncmp(m->fname, p, p2-p) == 0) {
					continue;
				}
			}

			if (verbose) {
				printf("%s(%d) : %ld : %s '", 
				       m->fname, 
				       line,
				       ftell(m->f), 
				       hashes.hash[i].note);
				for (p=buf, b=0; b<hashes.token_width; b++) {
					printf("%s ", p);
					p += strlen(p) + 1;
				}
				printf("'\n");
			}
			register_match(m->fname, line, &hashes.hash[i], ranges);
		}
	}
}

/*
  check one file for matches
*/
static void check_file(const char *fname)
{
	struct match_struct m;
	FILE *f;
	f = fopen(fname, "r");
	if (!f) {
		perror(fname);
		return;
	}

	m.f = f;
	m.fname = fname;

	/* notice that we always search with a skip of 1 - this is
	   important as otherwise the two sets of text can be
	   misaligned */
	gen_hashblocks(f, hashes.token_width, 1, hash_check, &m);

	fclose(f);

	register_match(NULL, 0, NULL, NULL);
}

/*
  scan a directory for matching text
*/
static void scan_dir(const char *dname)
{
	traverse_dir(dname, check_file, filename_pattern);
}

/*
  show help
*/
static void usage(void)
{
	printf("hashmatch version %s - Copyright Andrew Tridgell 2003\n", VERSION);
	printf("released under the GNU GPL v2 or later\n\n");
	printf("Usage: hashmatch [options] <hashfile> <dir>\n");
	printf("\nOptions: \n");
	printf("\t-m match_threshold  set match threshold (default %d)\n", 
	       options.match_threshold);
	printf("\t-f fuzz_threshold   set fuzz threshold (default %d)\n", 
	       options.fuzz_threshold);
	printf("\t-A                  avoid self matches\n");
	printf("\t-H                  produce HTML output\n");
	printf("\t-C                  produce curses output\n");
	printf("\t-p PATTERN          set filename wildcard pattern\n");
}

int main(int argc, char *argv[])
{
	int c;

	setlinebuf(stdout);

	options.fuzz_threshold = 5;
	options.match_threshold = 1;
	options.max_uses = 1;

	while ((c = getopt(argc, argv, "vhm:f:HCp:u:A")) != -1) {
		switch (c) {
		case 'm':
			options.match_threshold = atoi(optarg);
			break;

		case 'u':
			options.max_uses = atoi(optarg);
			break;

		case 'f':
			options.fuzz_threshold = atoi(optarg);
			break;

		case 'v':
			verbose++;
			break;

		case 'A':
			options.avoid_self_matches = 1;
			break;

		case 'H':
			options.html_format = 1;
			break;

		case 'C':
			options.curses_format = 1;
			break;

		case 'p':
			filename_pattern = strdup(optarg);
			break;

		case 'h':
		default:
			usage();
			exit(0);
		}
		
	}

	argv += optind;
	argc -= optind;

	if (argc != 2) {
		usage();
		exit(1);
	}

	if (options.html_format) {
		puts("<HTML><BODY>\n");
		puts("<pre>\n");
	}

	load_hashes(argv[0]);
	
	printf("match_threshold %d\n", options.match_threshold);
	printf("fuzz_threshold %d\n", options.fuzz_threshold);
	printf("max_uses %d\n", options.max_uses);
	printf("avoid_self_matches %d\n", options.avoid_self_matches);
	printf("token_width %d\n", hashes.token_width);
	printf("token_skip %d\n", hashes.token_skip);


	if (options.html_format) {
		printf("\nMatching tokens are shown in <font color=red>red</font>\n");
		puts("</pre>\n");
	}

	scan_dir(argv[1]);

	if (options.html_format) {
		puts("</BODY></HTML>\n");
	}
	
	return 0;
}
