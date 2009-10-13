#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>

/*
  I'm desperate ... an attempt to create C #defines from WSPP bitmaps

  This takes a text bitmap from WSPP on stdin and produces C defines
  on stdout

Here is an example from MS-DRSR section 5.37

  0    1    2   3   4   5   6   7   8   9 10 1      2   3   4   5   6   7   8   9 20 1      2   3   4   5   6   7   8   9 30 1


  A    G A      A W     I   P   M   A   T   C   G G     X   S   F   X   F   X   X   R   X   X   S   X   S   D   D   U N      S   G

  S    C    R   L   R   S   S   R   S   S   O   A   S       N   S       S           G           S       F   A   P   C   N    P   P

                L                   R               /       /   /       P                                   S   S

                /                   /               L       R   N

                D                   I               O       F   S

                R                   E


 X: Unused. MUST be zero and ignored.

 AS (DRS_ASYNC_OP): Perform the operation asynchronously.

 GC (DRS_GETCHG_CHECK): Treat ERROR_DS_DRA_REF_NOT_FOUND and
 ERROR_DS_DRA_REF_ALREADY_EXISTS as success for calls to IDL_DRSUpdateRefs (section
 4.1.26).

 AR (DRS_ADD_REF): Register a client DC for notifications of updates to the NC replica.

 ALL (DRS_SYNC_ALL): Replicate from all server DCs.

 DR (DRS_DEL_REF): Deregister a client DC from notifications of updates to the NC replica.

 WR (DRS_WRIT_REP): Replicate a writable replica, not a read-only partial replica or read-only full
 replica.

 IS (DRS_INIT_SYNC): Perform replication at startup.

 PS (DRS_PER_SYNC): Perform replication periodically.

 MR (DRS_MAIL_REP): Perform replication using SMTP as a transport.

 ASR (DRS_ASYNC_REP): Populate the NC replica asynchronously.

 IE (DRS_IGNORE_ERROR): Ignore errors.

 TS (DRS_TWOWAY_SYNC): Inform the server DC to replicate from the client DC.

 CO (DRS_CRITICAL_ONLY): Replicate only system-critical objects.

 GA (DRS_GET_ANC): Include updates to ancestor objects before updates to their descendants.

 GS (DRS_GET_NC_SIZE): Get the approximate size of the server NC replica.

 LO (DRS_LOCAL_ONLY): Perform the operation locally without contacting any other DC.

 SN (DRS_SYNC_BYNAME): Choose the source server by network name.

 RF (DRS_REF_OK): Allow the NC replica to be removed even if other DCs use this DC as a
 replication server DC.

 FS (DRS_FULL_SYNC_NOW): Replicate all updates in the replication cycle, even those that would
 normally be filtered.

 NS (DRS_NO_SOURCE): The NC replica has no server DCs.

 FSP (DRS_FULL_SYNC_PACKET): Replicate all updates in the replication request, even those that
 would normally be filtered.

 RG (DRS_REF_GCSPN): Requests that the server add an entry to repsTo for the client on the root
 object of the NC replica that is being replicated. When repsTo is set using this flag, the server
 contacts the client using GC SPN (section 2.2.3.2).

 SS (DRS_SPECIAL_SECRET_PROCESSING): Do not replicate attribute values of attributes that
 contain secret data.

 SF (DRS_SYNC_FORCED): Force replication, even if the replication system is otherwise disabled.

 DAS (DRS_DISABLE_AUTO_SYNC): Disable replication induced by update notifications.

 DPS (DRS_DISABLE_PERIODIC_SYNC): Disable periodic replication.

 UC (DRS_USE_COMPRESSION): Compress response messages.

 NN (DRS_NEVER_NOTIFY): Do not send update notifications.

 SP (DRS_SYNC_PAS): Expand the partial attribute set of the partial replica.

 GP (DRS_GET_ALL_GROUP_MEMBERSHIP): Replicate all kinds of group membership. If this flag
 is not present nonuniversal group membership will not be replicated.

I got the above from "pdftotext -layout [MS-DRSR].pdf"

This will produce:

#define BIT_DRS_ASYNC_OP                  0x00000001
#define BIT_DRS_GETCHG_CHECK              0x00000002
#define BIT_DRS_ADD_REF                   0x00000004
#define BIT_DRS_SYNC_ALL                  0x00000008
#define BIT_DRS_DEL_REF                   0x00000008
#define BIT_DRS_WRIT_REP                  0x00000010
#define BIT_DRS_INIT_SYNC                 0x00000020
#define BIT_DRS_PER_SYNC                  0x00000040
#define BIT_DRS_MAIL_REP                  0x00000080
#define BIT_DRS_ASYNC_REP                 0x00000100
#define BIT_DRS_IGNORE_ERROR              0x00000100
#define BIT_DRS_TWOWAY_SYNC               0x00000200
#define BIT_DRS_CRITICAL_ONLY             0x00000400
#define BIT_DRS_GET_ANC                   0x00000800
#define BIT_DRS_GET_NC_SIZE               0x00001000
#define BIT_DRS_LOCAL_ONLY                0x00001000
#define BIT_DRS_SYNC_BYNAME               0x00004000
#define BIT_DRS_REF_OK                    0x00004000
#define BIT_DRS_FULL_SYNC_NOW             0x00008000
#define BIT_DRS_NO_SOURCE                 0x00008000
#define BIT_DRS_FULL_SYNC_PACKET          0x00020000
#define BIT_DRS_REF_GCSPN                 0x00100000
#define BIT_DRS_SPECIAL_SECRET_PROCESSING 0x00800000
#define BIT_DRS_SYNC_FORCED               0x02000000
#define BIT_DRS_DISABLE_AUTO_SYNC         0x04000000
#define BIT_DRS_DISABLE_PERIODIC_SYNC     0x08000000
#define BIT_DRS_USE_COMPRESSION           0x10000000
#define BIT_DRS_NEVER_NOTIFY              0x20000000
#define BIT_DRS_SYNC_PAS                  0x40000000
#define BIT_DRS_GET_ALL_GROUP_MEMBERSHIP  0x80000000

*/

static int line_num;
static int verbose;

#define MAX_MAPPINGS 100
static int num_mappings;
static struct {
	const char *short_form;
	const char *long_form;
} mappings[MAX_MAPPINGS];
static int longest_mapping;

static void add_mapping(char *line)
{
	char *p;
	
	if (num_mappings == MAX_MAPPINGS) {
		printf("/* (%d) too many mappings! */\n", line_num);
		exit(1);
	}

	p = strchr(line, ':');
	if (p == NULL) return;
	*p = 0;
	p = strchr(line, '(');
	if (p == NULL) return;
	*p = 0;
	mappings[num_mappings].long_form = strdup(p+1);
	for (p=line; isspace(*p); p++) ;
	mappings[num_mappings].short_form = strdup(p);
	p = strchr(mappings[num_mappings].long_form, ')');
	if (p == NULL) return;
	*p = 0;

	p = strchr(mappings[num_mappings].short_form, ' ');
	if (p == NULL) return;
	*p = 0;

	if (strlen(mappings[num_mappings].long_form) > longest_mapping) {
		longest_mapping = strlen(mappings[num_mappings].long_form);
	}

	num_mappings++;	
}

static const char *map_name(const char *s)
{
	int i;
	for (i=0; i<num_mappings; i++) {
		if (strcmp(s, mappings[i].short_form) == 0) {
			return mappings[i].long_form;
		}
	}
	return s;
}

static char *remove_spaces(const char *s)
{
	char *ret = strdup(s);
	int i=0;
	while (*s) {
		if (!isspace(*s)) {
			ret[i++] = *s;
		}
		s++;
	}
	return ret;
}

#define MAX_LINES 50
#define MAX_WIDTH 200

static int nlines = 0;
static char lines[MAX_LINES][MAX_WIDTH];

static void shift_it(int lnum, int to, int from)
{
	int i;

	for (i=lnum; i<nlines; i++) {
		if (lines[i][from] == ' ') break;
		if (lines[i][to] != ' ') {
			printf("/* (line %d) Non-empty to position at line %d posn %d\n */", line_num, i, to);
			exit(1);
		}
		lines[i][to] = lines[i][from];
		lines[i][from] = ' ';
	}
}

static const char *prefix = "BIT_";
static int idl_format = 0;
static int little_endian = 0;

static int process_bitmap(const char *comment, FILE *f, char **next_section)
{
	char *p;
	int i;
	int max_len = 0;
	int bitpos;
	int in_mappings = 0;
	int more_sections = 0;

	num_mappings = 0;
	nlines = 0;
	longest_mapping = 0;

	while (fgets(lines[nlines], MAX_WIDTH-1, f)) {
		char *line = lines[nlines];

		line_num++;

		if (line[strlen(line)-1] == '\n') {
			line[strlen(line)-1] = 0;
		}

		if (isdigit(line[0])) {
			/* a new section */
			(*next_section) = strdup(line);
			more_sections = 1;
			break;
		}

		if (nlines == 0) {
			/* discard preamble lines */
			if (strncmp(remove_spaces(line), "0123456789", 10) != 0) {
				continue;
			}
		}

		/* detect the start of the descriptions */
		for (i=0; line[i]; i++) {
			if (line[i] != ' ' &&
			    !isdigit(line[i]) &&
			    !isupper(line[i])) {
				in_mappings = 1;
				break;
			}
		}

		if (in_mappings) {
			add_mapping(line);
			continue;
		}

		/* some bitmaps contain 10/20/30 instead of 0 */
		if ((p = strstr(line, "10 "))) {
			p[0] = ' ';
		}
		if ((p = strstr(line, "20 "))) {
			p[0] = ' ';
		}
		if ((p = strstr(line, "30 "))) {
			p[0] = ' ';
		}
		/* discard empty lines */
		if (strlen(line) > 0) {
			nlines++;
			if (strlen(line) > max_len) {
				max_len = strlen(line);
			}
		}
	}

	if (nlines < 2) {
		return more_sections;
	}

	/* pad lines with spaces if needed */
	for (i=0; i<nlines; i++) {
		if (strlen(lines[i]) < max_len) {
			memset(&lines[i][strlen(lines[i])], ' ', max_len - strlen(lines[i]));
		}
	}

	/* normalise the 2nd line */
	for (i=0; lines[1][i]; i++) {
		int tpos;

		if (lines[0][i] == ' ' && lines[1][i] == ' ') continue;

		if (lines[0][i] != ' ' && lines[1][i] != ' ') {
			/* already aligned */
			continue;
		}

		if (lines[0][i] == ' ') {
			/* we need to shift the next digit left */
			for (tpos = i+1; tpos<max_len; tpos++) {
				if (lines[0][tpos] != ' ') {
					break;
				}
			}
			if (tpos == max_len) {
				printf("/* (line %d) no digit match at pos %d */\n", line_num, i);
				exit(1);
			}
			lines[0][i] = lines[0][tpos];
			lines[0][tpos] = ' ';
			continue;
		}

		/* now we need to look for the missing tag either to
		 * the left or right. Try left first */
		for (tpos = i-1; tpos>=0; tpos--) {
			if (lines[0][tpos] != ' ') {
				/* already taken - give up going left */
				goto try_right;
			}
			if (lines[1][tpos] != ' ') {
				break;
			}
		}
		if (tpos >= 0) {
			shift_it(1, i, tpos);
			continue;
		}
try_right:
		for (tpos = i+1; tpos<max_len; tpos++) {
			if (lines[1][tpos] != ' ') {
				break;
			}
		}
		if (tpos < max_len) {
			shift_it(1, i, tpos);
			continue;
		} else {
			printf("/* (line %d) No match at position %d of line 1\n", line_num, i);
			for (i=0; i<nlines; i++) {
				printf("   line[%d]: %s\n", i, lines[i]);
			}
			printf("*/\n");
			exit(1);
		}
	}

	if (verbose) {
		printf("/* (line %d): \n", line_num); 
		for (i=0; i<nlines; i++) {
			printf("   line[%d]: %s\n", i, lines[i]);
		}
		printf("*/\n");
	}
	/* print the bits */
	bitpos = 0;
	if (comment) {
		printf("/* %s */\n", comment);
	}
	for (i=0; lines[0][i]; i++) {
		int n;
		char name[MAX_LINES+1] = "";
		char *t;

		if (lines[0][i] == ' ') continue;
		for (n=1; n<nlines; n++) {
			if (lines[n][i] == ' ') break;
			name[n-1] = lines[n][i];
		}
		for (t = strtok(name, "/"); t; t=strtok(NULL, "/")) {
			const char *mapped_name = map_name(t);
			unsigned value;

			if (little_endian) {
				int bpos, bnum = bitpos/8;
				bpos = (bnum*8) + (7 - (bitpos%8));
				value = (1U<<bpos);
			} else {
				value = (1U<<bitpos);
			}
			if (strcmp(t, "X") == 0) continue;
			if (idl_format) {
				printf("%s%-*s = 0x%08x,\n", prefix, longest_mapping, mapped_name, value);
			} else {
				printf("#define %s%-*s 0x%08x\n", prefix, longest_mapping, mapped_name, value);
			}
		}
		bitpos++;
	}

	return more_sections;
}


static void usage(void)
{
	printf("wspp_bits [options] <file>\n");
	printf("Options:\n");
	printf("\t-i           use IDL bitmap format\n");
	printf("\t-L           use little-endian bit order\n");
	printf("\t-p PREFIX    use the given prefix\n");
	printf("\t-v           increase verbosity\n");
}

int main(int argc, char * const argv[])
{
	int opt;
	FILE *f;
	char *sect_name = NULL;

	while ((opt = getopt(argc, argv, "vip:Lh")) != -1) {
		switch (opt) {
		case 'p':
			prefix = optarg;
			break;
		case 'i':
			idl_format = 1;
			break;
		case 'v':
			verbose++;
			break;
		case 'L':
			little_endian = 1;
			break;
		default:
			usage();
			exit(1);
		}
	}

	argv += optind;
	argc -= optind;

	if (argc < 1) {
		f = stdin;
	} else {
		f = fopen(argv[0], "r");
		if (f == NULL) {
			perror(argv[0]);
			exit(1);
		}
	}

	while (process_bitmap(sect_name, f, &sect_name)) ;

	return 0;
}
