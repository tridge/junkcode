/*****************************************************************************
a tokenised block hashing algorithm designed to find common pieces
of source code between two trees

Idea:

- tokenise input
- collect groups of N tokens, overlapping my M tokens
- hash each group

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

static int token_width=7, token_skip=5;
static int annotate;
static const char *filename_pattern = "*";

struct hash_info {
	const char *fname;
	FILE *in, *out;
};

/*
  write the hash to a file
*/
static void hash_write(const char *buf, const uint8 *md4, void *private, int line, struct char_range *ranges)
{
	struct hash_info *h = private;

	hash_print(h->out, md4);
	if (annotate) {
		fprintf(h->out," %s(%d:%ld:%ld)\n", 
			h->fname, line, ranges[0].start, 
			ranges[token_width-1].end - ranges[0].start);
	} else {
		fprintf(h->out,"\n");
	}
}


/* 
   form groups of token_width tokens, with a skip of token_skip, and write hashes
   to hashfile
*/
static void blockhash(FILE *infile, FILE *hashfile, struct hash_info *h)
{
	gen_hashblocks(infile, token_width, token_skip, hash_write, h);
}


/*
  show help
*/
static void usage(void)
{
	printf("Blockhash version %s - Copyright Andrew Tridgell 2003\n", VERSION);
	printf("released under the GNU GPL v2 or later\n\n");
	printf("Usage: blockhash [options] <dir>\n");
	printf("\nOptions: \n");
	printf("\t-w token_width (default %d)\n", token_width);
	printf("\t-s token_skip (default %d)\n", token_skip);
	printf("\t-a                  source filename enable annotation\n");
	printf("\t-p PATTERN          set filename wildcard pattern\n");
}

/*
  hash one file
*/
static void scan_fn(const char *fname)
{
	struct hash_info h;
	FILE *f = fopen(fname, "r");
	if (!f) {
		perror(fname);
		return;
	}
	h.in = f;
	h.fname = fname;
	h.out = stdout;
	blockhash(f, stdout, &h);
	fclose(f);
}

int main(int argc, char *argv[])
{
	int c;

	while ((c = getopt(argc, argv, "hw:s:ap:")) != -1) {
		switch (c) {
		case 'w':
			token_width = atoi(optarg);
			break;
		case 's':
			token_skip = atoi(optarg);
			break;
		case 'a':
			annotate = 1;
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
	
	if (token_width < token_skip || token_skip <= 0) {
		printf("Invalid width/skip parameters\n");
		usage();
		exit(1);
	}

	argv += optind;
	argc -= optind;

	if (argc != 1) {
		usage();
		exit(1);
	}

	/* deliberately arrange things so that 'sort -r' will leave
	   the format intact while removing the hash ordering
	   information from the file */
	printf("version %s of blockhash\n", VERSION);
	printf("token_width %d\n", token_width);
	printf("token_skip %d\n", token_skip);

	traverse_dir(argv[0], scan_fn, filename_pattern);

	return 0;
}

