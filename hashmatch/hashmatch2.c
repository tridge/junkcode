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


static struct {
	int line_fuzz;
	int min_matches;
	int avoid_self_matches;
	int max_uses;
} options;


struct hash_handle {
	FILE *f;
	char *last_hashs;
	char *hashs;
	char *annotation;
	int line;
	long ofs, len;
	int token_width;
	int token_skip;
	int repeat_count;
	const char *fname;
};


static struct {
	int num_matches;
	int num_allocated;
	struct match {
		char *name[2];
		int line[2];
		long ofs[2];
		long len[2];
		int repeat_count;
	} *m;
} reg_matches;


/*
  advance a ptr until newline
 */
static long advance_ptr(char *p, long ofs, int inc, int n)
{
	while (n--) {
		while (ofs > 0 && p[ofs] != 0) {
			ofs += inc;
			if (p[ofs] == '\n') break;
		}
	}
	return ofs;
}

/*
  see if a character offset is in a match range
*/
static int check_range(long ofs, int start, int n, int idx)
{
	int i;
	for (i=start;i<start+n;i++) {
		if (reg_matches.m[i].ofs[idx] <= ofs &&
		    reg_matches.m[i].ofs[idx] + reg_matches.m[i].len[idx] > ofs) {
			return 1;
		}
		if (reg_matches.m[i].ofs[idx] > ofs) {
			return 0;
		}
	}
	return 0;
}

/*
  compare two match records
*/
static int match_compare(struct match *m1, struct match *m2)
{
	int v;

	v = strcmp(m1->name[0], m2->name[0]);
	if (v != 0) return v;

	if (m1->ofs[0] != m2->ofs[0]) {
		return m1->ofs[0] - m2->ofs[0];
	}

	v = strcmp(m1->name[1], m2->name[1]);
	if (v != 0) return v;

	if (m1->ofs[1] != m2->ofs[1]) {
		return m1->ofs[1] - m2->ofs[1];
	}
	
	return 0;
}


/*
  compare two match records
*/
static int match_compare2(struct match *m1, struct match *m2)
{
	int v;

	v = strcmp(m1->name[1], m2->name[1]);
	if (v != 0) return v;

	if (m1->ofs[1] != m2->ofs[1]) {
		return m1->ofs[1] - m2->ofs[1];
	}
	
	v = strcmp(m1->name[0], m2->name[0]);
	if (v != 0) return v;

	if (m1->ofs[0] != m2->ofs[0]) {
		return m1->ofs[0] - m2->ofs[0];
	}

	return 0;
}




/*
  dump a preformatted range of a file
*/
static void dump_preformatted(const char *fname, int start, int n, int idx)
{
	int i;
	char *p;
	int highlight = 0;
	long ofs1, ofs2;

	if (strcmp(fname, "-") == 0) {
		return;
	}

	if (strcmp(reg_matches.m[start].name[idx], 
		   reg_matches.m[start+n-1].name[idx]) != 0) {
		int base = start;
		for (i=start+1;i<start+n;i++) {
			if (strcmp(reg_matches.m[i-1].name[idx],
				   reg_matches.m[i].name[idx]) != 0) {
				printf("<font color=green>%s:%d</font><p>\n",
				       reg_matches.m[base].name[idx],
				       reg_matches.m[base].line[idx]);
				dump_preformatted(reg_matches.m[i-1].name[idx], base, i-base, idx);
				base = i;
			}
		}
		printf("<font color=green>%s:%d</font><p>\n",
		       reg_matches.m[base].name[idx],
		       reg_matches.m[base].line[idx]);
		dump_preformatted(reg_matches.m[i-1].name[idx], base, i-base, idx);
		return;
	}

	p = load_file(fname);
	if (!p) return;

	ofs1 = reg_matches.m[start].ofs[idx];
	ofs2 = reg_matches.m[start+n-1].ofs[idx] + reg_matches.m[start+n-1].len[idx];

	/* move offsets to start of lines */
	ofs1 = advance_ptr(p, ofs1, -1, 4);
	ofs2 = advance_ptr(p, ofs2, 1, 4);

	printf("<pre>\n");
	for (i=ofs1;i<=ofs2;i++) {
		int in_range = check_range(i, start, n, idx);
		if (in_range && !highlight) {
			highlight = 1;
			printf("<font color=red>");
		}
		if (!in_range && highlight) {
			highlight = 0;
			printf("</font>");
		}
		if (p[i] == '<') {
			printf("&lt;");
		} else {
			putchar(p[i]);
		}
	}
	if (highlight) {
		printf("</font>");
	}
	printf("</pre>\n");

	free(p);
}

/*
  display a matching area
*/
static void show_match(int start, int n)
{
	char *fname1 = reg_matches.m[start].name[0];
	char *fname2;

	printf("<hr><br>\n");
	printf("<font color=red>%s:%d</font>    <font color=green>%s:%d</font><p>\n",
	       reg_matches.m[start].name[0],
	       reg_matches.m[start].line[0],
	       reg_matches.m[start].name[1],
	       reg_matches.m[start].line[1]);

	printf("<table border=1 CELLSPACING=10 CELLPADDING=10>");
	printf("<tr>\n");
	printf("<td valign=top>\n");
	dump_preformatted(fname1, start, n, 0);
	printf("</td>\n");

	qsort(reg_matches.m + start, n, 
	      sizeof(reg_matches.m[0]), (comparison_fn_t)match_compare2);

	printf("<td>\n");
	fname2 = reg_matches.m[start].name[1];
	dump_preformatted(fname2, start, n, 1);
	printf("</td>\n");

	printf("</tr>\n");
	printf("</table>\n");

}


/*
  process found matches
*/
static void process_matches(struct hash_handle *hh1, struct hash_handle *hh2)
{
	int i, j;

	printf("processing %d matches\n", reg_matches.num_matches);

	qsort(reg_matches.m, reg_matches.num_matches, 
	      sizeof(reg_matches.m[0]), (comparison_fn_t)match_compare);

	for (i=0;i<reg_matches.num_matches;) {
		int min_repeat = options.max_uses+1;
		j=1;
		while (i+j < reg_matches.num_matches &&
		       strcmp(reg_matches.m[i].name[0], reg_matches.m[i+j].name[0]) == 0 &&
		       reg_matches.m[i+j].line[0] <= 
		       reg_matches.m[i+j-1].line[0] + options.line_fuzz) {
			if (reg_matches.m[i+j].repeat_count < min_repeat) {
				min_repeat = reg_matches.m[i+j].repeat_count;
			}
			j++;
		}

		if (min_repeat <= options.max_uses &&
		    j > options.min_matches) {
			show_match(i, j);
		}

		i += j;
	}
}

/*
  open up a hash file
*/
static struct hash_handle *hh_open(const char *fname)
{
	char version[11];
	struct hash_handle *hh;
	hh = calloc(1, sizeof(*hh));
	if (!hh) return NULL;
	hh->f = fopen(fname, "r");
	if (!hh->f) {
		free(hh);
		return NULL;
	}

	hh->fname = fname;

	if (fscanf(hh->f, "version %10s of blockhash\n", version) != 1 ||
	    strcmp(version, VERSION) != 0) {
		fprintf(stderr, "Bad version tag '%s' in hashfile %s\n", version, fname);
		exit(1);
	}

	if (fscanf(hh->f, "token_width %d\n", &hh->token_width) != 1 ||
	    fscanf(hh->f, "token_skip %d\n", &hh->token_skip) != 1) {
		fprintf(stderr, "Failed to read token_width/token_skip tags in %s\n", 
			fname);
		exit(1);
	}

	if (hh->token_skip > hh->token_width || hh->token_skip <= 0) {
		fprintf(stderr, "Bad token_width/token_skip tags in %s\n", fname);
		exit(1);
	}

	return hh;
}

/*
  move to the next hash in a hash file
  return 0 on success, -1 on failure
*/
static int hh_next(struct hash_handle *hh)
{
	char buf[1024];
	char *p;

	if (!fgets(buf, sizeof(buf), hh->f)) {
		return -1;
	}

	if (buf[strlen(buf)-1] == '\n') {
		buf[strlen(buf)-1] = 0;
	}

	p = strchr(buf,' ');
	if (p) {
		*p = 0;
	}

	if (hh->hashs && strcmp(buf, hh->hashs) == 0) {
		hh->repeat_count++;
	} else {
		hh->repeat_count = 0;
	}

	if (hh->last_hashs) {
		free(hh->last_hashs);
	}
	hh->last_hashs = hh->hashs;

	hh->hashs = strdup(buf);

	if (hh->last_hashs && strcmp(hh->last_hashs, hh->hashs) < 0) {
		fprintf(stderr, "Hash file is not sorted! Use 'sort -r' on blockhash output \n");
		exit(1);
	}

	if (hh->annotation) {
		free(hh->annotation);
	}
	
	if (p) {
		hh->annotation = strdup(p+1);
	} else {
		hh->annotation = strdup("-");
	}

	hh->line = -1;

	if (hh->annotation) {
		p = strchr(hh->annotation, '(');
	}
	if (p) {
		*p = 0;
		if (sscanf(p+1, "%d:%ld:%ld", &hh->line, &hh->ofs, &hh->len) != 3) {
			fprintf(stderr, "Badly formatted annotation '%s'\n", p+1);
			exit(1);
		}
	}

	return 0;	
}

/*
  have we run out of hashes?
  return non-zero if out of hashes, zero if there are more
*/
static int hh_eof(struct hash_handle *hh)
{
	return feof(hh->f);
}

/*
  advance the hash until we know if we either find a match with hh2, or
  we know that we won't find a match (assumes ordered hash)
  return 0 if we found a match, -1 if not
*/
static int hh_next_match(struct hash_handle *hh, struct hash_handle *hh2)
{
	if (!hh->hashs && hh_next(hh) == -1) {
		return -1;
	}

	while (strcmp(hh->hashs, hh2->hashs) > 0) {
		if (hh_next(hh) == -1) return -1;
	}

	if (strcmp(hh->hashs, hh2->hashs) == 0) {
		return 0;
	}

	return -1;
}


/*
  register a match that has been found
*/
static void register_match(struct hash_handle *hh1, struct hash_handle *hh2)
{
	if (options.avoid_self_matches &&
	    strcmp(hh1->annotation, hh2->annotation) == 0) {
		return;
	}

	if (reg_matches.num_matches + 1 > reg_matches.num_allocated) {
		reg_matches.num_allocated = (reg_matches.num_allocated+1000)*2;
		reg_matches.m = realloc(reg_matches.m, 
					 sizeof(reg_matches.m[0]) *
						reg_matches.num_allocated);
		if (!reg_matches.m) {
			fprintf(stderr, "Insufficient memory for %d matches!\n",
				reg_matches.num_matches+1);
			exit(1);
		}
	}

	reg_matches.m[reg_matches.num_matches].name[0] = NULL;
	reg_matches.m[reg_matches.num_matches].name[1] = NULL;
	if (hh1->annotation) {
		if (reg_matches.num_matches > 0 &&
		    strcmp(hh1->annotation, reg_matches.m[reg_matches.num_matches-1].name[0]) == 0) {
			reg_matches.m[reg_matches.num_matches].name[0] = 
				reg_matches.m[reg_matches.num_matches-1].name[0];
		} else {
			reg_matches.m[reg_matches.num_matches].name[0] = strdup(hh1->annotation);
		}
	}
	if (hh2->annotation) {
		if (reg_matches.num_matches > 0 &&
		    strcmp(hh2->annotation, reg_matches.m[reg_matches.num_matches-1].name[1]) == 0) {
			reg_matches.m[reg_matches.num_matches].name[1] = 
				reg_matches.m[reg_matches.num_matches-1].name[1];
		} else {
			reg_matches.m[reg_matches.num_matches].name[1] = strdup(hh2->annotation);
		}
	}
	reg_matches.m[reg_matches.num_matches].line[0] = hh1->line;
	reg_matches.m[reg_matches.num_matches].line[1] = hh2->line;
	reg_matches.m[reg_matches.num_matches].ofs[0] = hh1->ofs;
	reg_matches.m[reg_matches.num_matches].ofs[1] = hh2->ofs;
	reg_matches.m[reg_matches.num_matches].len[0] = hh1->len;
	reg_matches.m[reg_matches.num_matches].len[1] = hh2->len;

	reg_matches.num_matches++;
}

/*
  perform a 3-way match between 3 hash sources
*/
static void three_way(struct hash_handle *hh1, struct hash_handle *hh2,
		      struct hash_handle *hh_exclude)
{
	while (hh_next(hh1) == 0) {
		if (hh_next_match(hh2, hh1) == 0 &&
		    (!hh_exclude || hh_next_match(hh_exclude, hh1) != 0)) {
			register_match(hh1, hh2);
		}
		if (hh_eof(hh2)) break;
	}

	process_matches(hh1, hh2);	
}

/*
  show help
*/
static void usage(void)
{
	printf("hashmatch version %s - Copyright Andrew Tridgell 2003\n", VERSION);
	printf("released under the GNU GPL v2 or later\n\n");
	printf("Usage: hashmatch [options] <hashfile1> <hashfile2>\n");
	printf("Options:\n");
	printf("  -m min_matches       set min matching lines (default %u)\n", 
	       options.min_matches);
	printf("  -f line_fuzz         set line fuzz factor (default %u)\n",
		options.line_fuzz);
	printf("  -A                   avoid self matches\n");
}

int main(int argc, char *argv[])
{
	int c;
	const char *fname1, *fname2, *fname_exclude=NULL;
	struct hash_handle *hh1, *hh2, *hh_exclude=NULL;

	setlinebuf(stdout);

	options.line_fuzz = 6;
	options.min_matches = 2;

	while ((c = getopt(argc, argv, "hx:Af:m:u:")) != -1) {
		switch (c) {
		case 'x':
			fname_exclude = optarg;
			break;
		case 'f':
			options.line_fuzz = atoi(optarg);
			break;
		case 'm':
			options.min_matches = atoi(optarg);
			break;
		case 'u':
			options.max_uses = atoi(optarg);
			break;
		case 'A':
			options.avoid_self_matches = 1;
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

	fname1 = argv[0];
	fname2 = argv[1];

	hh1 = hh_open(fname1);
	if (!hh1) {
		perror(fname1);
		exit(1);
	}

	hh2 = hh_open(fname2);
	if (!hh2) {
		perror(fname2);
		exit(1);
	}

	if (hh1->token_width != hh2->token_width) {
		fprintf(stderr,"Token widths do not match! %s=%d %s=%d\n", 
			hh1->fname, hh1->token_width,
			hh2->fname, hh2->token_width);
		exit(1);
	}

	if (fname_exclude) {
		hh_exclude = hh_open(fname_exclude);
		if (!hh_exclude) {
			perror(fname_exclude);
			exit(1);
		}
	}

	three_way(hh1, hh2, hh_exclude);

	return 0;
}
