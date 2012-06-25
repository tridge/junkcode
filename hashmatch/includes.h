#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <fnmatch.h>

#define MAX_TOKEN_SIZE 100
#define VERSION "1.1"

typedef unsigned int uint32;
typedef unsigned char uint8;


struct char_range {
	long start, end;
};

void mdfour(unsigned char *out, const unsigned char *in, int n);
int file_size(const char *fname);
int file_lines(const char *fname);
void hash_print(FILE *f, const uint8 *buf);
void gen_hashblocks(FILE *infile, int token_width, int token_skip, 
		    void (*fn)(const char *, const uint8 *, void *, int, struct char_range *),
		    void *private);
void traverse_dir(const char *dir, void (*fn)(const char *), const char *pattern);
void dump_lines(const char *fname, int start, int end, 
		int num_ranges, struct char_range *ranges,
		const char *h_start, const char *h_end);
char *load_file(const char *fname);
