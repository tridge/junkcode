/* 
a quick hack to test an idea for 16 bit BMG string searches

Inspired by the fact that Padre is slow when lots of alternates are
specified in a query

This method reduces the density of zero entries in the delta_table and
also makes the list of candidate targets much smaller when a zero
entry is hit.

It still needs a lot of optimisation. In particular it needs to be
hard wired for 16 bit operation. Currently it can compile for ngraph
based matching, although n>2 would be silly.

Sample usage:

compile it with "gcc -O2 -o bmg16 -DTAG=short bmg.c"

run it with "bmg16 'hello|goodbye|yellow|blue' /usr/dict/words"

Andrew Tridgell
30/3/95
*/

#include <stdio.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <string.h>


/* this determines if the algorithm is 8 or 16 bit - it works for both */
#ifndef TAG
#define TAG short
#endif

typedef unsigned TAG tag;
typedef unsigned char uchar;


#define TABLE_SIZE (1<<(sizeof(tag)*8))
#define DELIMITERS "|"
#define MAX_ALTERNATES 255
#define AVG_ALTERNATES 4
#define PUNCTUATION " \t,;:.\"'`"
#define CASE_MASK (~(1<<5))

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

/* this is a case insensitive character match */
#define CHAR_EQUAL(c1,c2) (((c1)&CASE_MASK) == ((c2)&CASE_MASK))

static uchar delta_table[TABLE_SIZE];
static uchar target_list[TABLE_SIZE][AVG_ALTERNATES];
static uchar target_list_len[TABLE_SIZE];
static uchar *targets[MAX_ALTERNATES];
static int target_lengths[MAX_ALTERNATES];
static int num_targets = 0;
static int min_target_length = (1<<30);

/* extract a tag value from a buffer. Obviously if the tag size
   were fixed (8 or 16 bit) this would be much faster */
static inline tag tag_value(uchar *buf)
{
  int i;
  unsigned int ret=0;
  for (i=0;i<sizeof(tag);i++)
    ret |= (buf[i]<<(8*i));
  return(ret);
}

/* check if two characters match using the rules for
   punctuation */
static inline int uchar_match(uchar c,uchar b)
{
  return(CHAR_EQUAL(c,b) || (b==' ' && strchr(PUNCTUATION,c)));
}

/* check if two tags are equal. This is quite general, and slow */
static inline int tag_equal(tag t,uchar *buf,int offset)
{
  int i;
  if (offset>0) offset=0;
  for (i=-offset;i<sizeof(tag);i++)
    if (!uchar_match((t>>(8*i))&0xFF,buf[i])) return(0);
  return(1);
}

/* compare two strings */
static inline int strnequal(uchar *s1,uchar *s2,int n)
{
  while (n--) {
    if (!uchar_match(*s2,*s1)) return(0);
    s1++; s2++;
  }
  return(1);
}


/* generate a table of what chars occur at all in the targets */
static void generate_char_table(char *char_table)
{
  int has_space=0;
  uchar c;
  int i,j;
  uchar *p = (uchar *)PUNCTUATION;

  memset(char_table,0,256);
  for (i=0;i<num_targets;i++)
    for (j=0;j<target_lengths[i];j++) {
      c = targets[i][j];      
      if (c == ' ') 
	has_space = 1;
      else
	char_table[c & CASE_MASK] = char_table[c | ~CASE_MASK] = 1;
    }
  if (has_space)
    for (i=0;i<strlen(p);i++)
      char_table[p[i]] = 1;
}



/* check if a tag is possible */
static inline int valid_tag(char *char_table,tag t)
{
  if (!char_table[(t>>(8*(sizeof(tag)-1)))&0xFF]) return(0);
  return(1);
}


/* build the tables needed for the bmg search */
void bmg_build_table(uchar *target_string)
{
  uchar *p = (uchar *)strdup(target_string);
  uchar *tok;
  int i,j;
  int t;
  char char_table[256];

  num_targets = 0;

  /* parse the command */
  for (tok=strtok(p,DELIMITERS); tok; tok = strtok(NULL,DELIMITERS)) {
    targets[num_targets] = strdup(tok);
    target_lengths[num_targets] = strlen(tok);
    min_target_length = MIN(min_target_length,target_lengths[num_targets]);
    num_targets++;
  }
  free(p);

  if (min_target_length < sizeof(tag))
    printf("Error! you have a target smaller than the tag\n");

  if (!num_targets) return;

  generate_char_table(char_table);

  /* built the bmg table - this is a mess */
  for (t=0;t<TABLE_SIZE;t++) {

    delta_table[t] = min_target_length;
    target_list_len[t] = 0;

    if (!valid_tag(char_table,t)) 
      continue;

    for (i=0;i<num_targets;i++)
      {
	for (j=(target_lengths[i]-sizeof(tag));j+sizeof(tag)>=1;j--)
	  if (tag_equal(t,targets[i]+j,j)) 
	    {
	      delta_table[t] = MIN(delta_table[t],
				   target_lengths[i]-(j+sizeof(tag)));
	      if (target_list_len[t] < AVG_ALTERNATES) {
		target_list[t][target_list_len[t]] = i;
		target_list_len[t]++;		
	      }	else {
		target_list_len[t] = MAX_ALTERNATES;		
	      }
	      break;
	    }
      }
  }  
}


/* some counters to see how we did */
static int stringops=0;
static int comparisons=0;

/* check if a proposed match is valid */
static inline int check_target(uchar *buf,int pos,int i)
{
  int posi = pos-target_lengths[i];
  if (posi<0) return(0);
  stringops++;
  if (strnequal(targets[i],buf+posi,target_lengths[i])) {
    printf("Found target %d (%s) at %d\n",i,targets[i],posi);
    return(1);
  }
  return(0);
}


/* the search proper - it takes a buffer and finds the targets previously
   specified when building the bmg tables */
void bmg_search(uchar *buf,int len)
{
  int pos = 0;
  int found = 0;
  int hits=0;
  int i;

  stringops=0;
  comparisons=0;

  for (pos=min_target_length; pos <= len;) 
    {
      tag v = tag_value(buf+pos-sizeof(tag));
      int skip = delta_table[v];
      comparisons++;
      if (skip) {
	pos += skip;
	continue;
      }
    
      if (target_list_len[v] <= AVG_ALTERNATES)
	for (found=0,i=0;i<target_list_len[v] && !found;i++)
	  found = check_target(buf,pos,target_list[v][i]);
      else
	for (found=0,i=0;i<num_targets && !found;i++)
	  found = check_target(buf,pos,i);

      if (found) hits++;
      pos++;
    }

  printf("\nGot %d hits with %d comparisons and %d stringops\n",
	 hits,comparisons,stringops);  
}


/* a simple harness to test the above */
int main(int argc,char *argv[])
{
  uchar *target;
  uchar *buf;
  char *fname;
  int fd;
  int len;

  if (argc < 3) {
    printf("Usage: bmg string|string|string... file\n");
    exit(1);
  }
   
  target = (uchar *)argv[1];
  fname = argv[2];
  fd = open(fname,O_RDONLY);
  if (fd < 0) {
    printf("couldn't open %s\n",fname);
    exit(1);
  }
  len = lseek(fd,0,SEEK_END);
  lseek(fd,0,SEEK_SET);
  buf = (uchar *)malloc(len);
  if (!buf) {
    printf("failed to alloc buffer\n");
    exit(1);
  }
  len = read(fd,buf,len);
  close(fd);
  printf("Loaded %d bytes\n",len);

  bmg_build_table(target);
  printf("Finished building tables\n");
  bmg_search(buf,len);
  return(0);
}

