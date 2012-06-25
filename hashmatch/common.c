/*****************************************************************************
common routines for blockhash

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

static int line_number;

#define iscword(c) (isalnum(c) || ((c) == '_'))

/*
  pull characters from a file, normalise the input and produce a token
  'tok' must be at least of size MAX_TOKEN_SIZE
*/
static void fetch_token(FILE *f, char *tok)
{
	int len = 0;
	int c;
	*tok = 0;
	while (len < (MAX_TOKEN_SIZE-1) && (c = fgetc(f)) != EOF) {
		if (c == '\n') line_number++;

		if (c == '_') {
			continue;
		}

		if (isspace(c)) {
			if (len == 0) continue;
			if (!iscword(tok[len-1])) {
				continue;
			}
			break;
		}

		tok[len++] = toupper(c);

		if (len > 1 && iscword(c) != iscword(tok[len-2])) {
			ungetc(c, f);
			len--;
			break;
		}
	}

	/* terminate the string, just for simplicity */
	tok[len] = 0;
}

/*
  pull characters from a file, normalise the input and produce a token
  'tok' must be at least of size MAX_TOKEN_SIZE
*/
static void fetch_token1(FILE *f, char *tok)
{
	int len = 0;
	int c;
	*tok = 0;
	while (len < (MAX_TOKEN_SIZE-1) && (c = fgetc(f)) != EOF) {
		if (c == '\n') line_number++;

		if (isspace(c)) {
			/* we swallow leading spaces, and break on
			   embedded spaces */
			if (len == 0) continue;
			break;
		}
		tok[len++] = c;
		/* break on any non alnum character, but not repeated characters */
		if (!isalnum(c) && c != '_' && len > 1 && c != tok[len-2]) {
			ungetc(c, f);
			len--;
			break;
		}
	}

	/* terminate the string, just for simplicity */
	tok[len] = 0;
}

/*
  generate the hash blocks from a file and call fn() on each block
*/
void gen_hashblocks(FILE *infile, int token_width, int token_skip, 
		    void (*fn)(const char *, const uint8 *, void *, int, struct char_range *),
		    void *private)
{
	char buffer[MAX_TOKEN_SIZE * token_width];
	char *p = buffer;
	int buflen=0, i, tokskip;
	struct char_range ranges[token_width];

	line_number = 1;

	/* fetch the initial tokens */
	for (i=0;i<token_width && !feof(infile);i++) {
		int len;

		ranges[i].start = ftell(infile);
		fetch_token(infile, p);
		ranges[i].end = ftell(infile);

		len = strlen(p) + 1;
		buflen += len;
		p += len;
	}

	while (!feof(infile)) {
		uint8 out[16];

		mdfour(out, (unsigned char *)buffer, buflen);

		fn(buffer, out, private, line_number, ranges);

		/* skip past some tokens */
		tokskip = 0;
		p = buffer;

		for (i=0;i<token_skip;i++) {
			int len = strlen(p)+1;
			tokskip += len;
			p += len;
		}

		memmove(ranges, ranges + token_skip, sizeof(ranges[0]) * (token_width - token_skip));

		memmove(buffer, buffer+tokskip, buflen-tokskip);
		buflen -= tokskip;
		
		/* and fetch some more */
		p = buffer + buflen;
		for (i=0;i<token_skip && !feof(infile);i++) {
			int len;

			ranges[i + (token_width-token_skip)].start = ftell(infile);
			fetch_token(infile, p);
			ranges[i + (token_width-token_skip)].end = ftell(infile);
			len = strlen(p) + 1;
			buflen += len;
			p += len;
		}		
	}
}



/*
  return file size or -1
*/
int file_size(const char *fname)
{
	struct stat st;
	if (stat(fname, &st) != 0) {
		return -1;
	}
	return st.st_size;
}

/*
  return number of lines in a file
*/
int file_lines(const char *fname)
{
	FILE *f = fopen(fname, "r");
	int lines = 0;
	char buf[1000];

	if (!f) return -1;

	while (fgets(buf, sizeof(buf)-1, f)) {
		lines++;
	}
	fclose(f);
	return lines;
}


/*
  print a hash
*/
void hash_print(FILE *f, const uint8 *buf)
{
	/* easy way to be machine independent */
	fprintf(f, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
		buf[0], buf[1], buf[2], buf[3],
		buf[4], buf[5], buf[6], buf[7],
		buf[8], buf[9], buf[10], buf[11],
		buf[12], buf[13], buf[14], buf[15]);
}

/* 
   recursive directory traversal
   fn() is called on all files/dirs in the tree
*/
void traverse_dir(const char *dir, void (*fn)(const char *), const char *pattern)
{
	DIR *d;
	struct dirent *de;

	d = opendir(dir);
	if (!d) return;

	while ((de = readdir(d))) {
		char *fname = NULL;
		struct stat st;

		if (strcmp(de->d_name,".") == 0) continue;
		if (strcmp(de->d_name,"..") == 0) continue;

		asprintf(&fname, "%s/%s", dir, de->d_name);
		if (lstat(fname, &st)) {
			perror(fname);
			free(fname);
			continue;
		}

		if (S_ISDIR(st.st_mode)) {
			traverse_dir(fname, fn, pattern);
		} else if (S_ISREG(st.st_mode) && 
			   fnmatch(pattern, fname, 0) == 0) {
			fn(fname);
		}

		free(fname);
	}

	closedir(d);
}

/*
  see if a file pointer is in a range
*/
static int check_range(FILE *f, int num_ranges, struct char_range *ranges)
{
	long ofs = ftell(f);
	int i;
	for (i=0;i<num_ranges;i++) {
		if (ofs > ranges[i].start && ofs <= ranges[i].end) {
			return 1;
		}
	}
	return 0;
}

/*
  dump a range of lines from a file
*/
void dump_lines(const char *fname, int start, int end,
		int num_ranges, struct char_range *ranges,
		const char *h_start, const char *h_end)
{
	FILE *f;
	int line = 1;
	int c, highlight;
#define start_highlight() do { fputs(h_start, stdout); 	highlight = 1; } while (0)
#define end_highlight() do { fputs(h_end, stdout); 	highlight = 0; } while (0)

#if 0
	printf("%d ranges\n", num_ranges);
	for (i=0;i<num_ranges;i++) {
		printf("  %ld %ld\n", ranges[i].start, ranges[i].end);
	}
#endif

	f = fopen(fname,"r");
	if (!f) {
		perror(fname);
		return;
	}

	highlight = 0;

	fputc('\n', stdout);

	if (start == 1) {
		printf("\t\t|  ");
	}

	while ((c = fgetc(f)) != EOF) {
		if (c == '\n' && highlight) {
			end_highlight();
		}
		if (line >= start-1) {
			int in_range = check_range(f, num_ranges, ranges);
			if (in_range && !highlight && c != '\n') {
				start_highlight();
			} 
			if (!in_range && highlight) {
				end_highlight();
			} 
			if (c == '\t') {
				printf("        ");
			} if (c == '<' && h_start[0] == '<') {
				printf("&lt;");
			} else {
				fputc(c, stdout);
			}
		}
		if (c == '\n') {
			line++;
			if (line > end+1 &&
			    ftell(f) > ranges[num_ranges-1].end + 20) break;

			if (ftell(f) > ranges[0].start - 20) {
				start = line;
			}

			if (line >= start-1) {
				if (highlight) {
					end_highlight();
				}
				printf("\t\t|  ");
			}
		}
	}
	fclose(f);
	fputc('\n', stdout);
	fputc('\n', stdout);

	if (highlight) {
		end_highlight();
	}
}


/*
  load a file from disk - null terminate it as a string
*/
char *load_file(const char *fname)
{
	struct stat st;
	int fd;
	char *p;

	fd = open(fname, O_RDONLY);
	if (fd == -1) {
		perror(fname);
		return NULL;
	}
	if (fstat(fd, &st) != 0) {
		close(fd);
		return NULL;
	}

	p = malloc(st.st_size + 1);
	if (!p) {
		close(fd);
		fprintf(stderr,"Out of memory loading %s\n", fname);
		return NULL;
	}

	if (read(fd, p, st.st_size) != st.st_size) {
		fprintf(stderr,"Error loading %s\n", fname);
		close(fd);
		free(p);
		return NULL;		
	}
	p[st.st_size] = 0;

	close(fd);
	
	return p;
}
