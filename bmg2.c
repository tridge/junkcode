/* COPYRIGHT (C) The Australian National University 1995 */
/*************************************************************************/
/*				bmg2.c				           */
/*			Digraph Boyer-Moore-Gosper algorithm               */
/*									   */
/***************************************************************************/

/* $Id: bmg2.c,v 1.1 2000/10/10 02:16:17 tridge Exp $ */

/* 
a digraph version of the bmg algorithm

Andrew Tridgell
30/3/95

Last modified 5/5/95

This module has the following interface:

int bmg_build_table(uchar *target_string,uchar *char_map,int wsmode);
void bmg_search(uchar *buf,int len,void (*fn)());

You call bmg_build_table() with the list of search terms, separated by
| The char_map[] is a character array which maps characters from the
data into characters in the search string. If this is null then a
default mapping is performed. The default mapping maps all non
alphanumeric characters to a space and other characters to themselves.

If wsmode is true then the search routine will only find words that begin
with the target strings. If wsmode is 0 then it will accept the
targets at any alignment.

bmg_search() is called with a pointer to the data, the length of the
data and a function to call when a match is found. This function will
be called with a pointer to the point at which the match was
found. When this function returns the search will automatically
continue from where it left off, calling fn() at each matching string.

Calling bmg_build_table() with a NULL target_string will free the tables
that are created by bmg_build_table().
*/

#include <stdio.h>
#include <ctype.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#if !AJT
#include "const.h"
#endif

typedef unsigned short tag;

#define TAB14 0
#define TAB16 0

/* if you have targets longer than 255 chars they will be truncated */
#define MAX_TARGET_LENGTH 255

/* this is what is used to delimit search alternates in the target
   string passed to the bmg_build_table() routine */
#define DELIMITERS "|"
#define SPACE_CHAR ' '

/* the following defines are optimisation constants. Leave them
   alone unless you know the code */
#define DIGRAPH_THRESHOLD 3

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif


struct target {
  int next;
  u_char *str;
  u_char length;
};

static u_char space_char = SPACE_CHAR;
static u_char char_bits = 0;
static int max_char = 0;
static int table_size = 0;
static u_char *delta_table = NULL;
#if TAB14 || TAB16
static u_char *delta_table2 = NULL;
#endif
static tag *tag_table = NULL;
static struct target *targets = NULL;
static tag num_targets = 0;
static int min_target_length = MAX_TARGET_LENGTH;
static int max_target_length = 0;
static tag char_table[256];
static tag char_table_shifted[256];
static int wsmode=0;
static int digraph=1;
static int tagsize=2;

#if TAB14
#define TAGV2(buf) ((((tag)(((u_char *)buf)[1])&0x7F)<<7)|(((u_char *)buf)[0]&0x7f))
#endif
#if TAB16
#define TAGV2(buf) ((((tag)(((u_char *)buf)[1]))<<8)|(((u_char *)buf)[0]))
#endif

/* extract a digraph tag value from a buffer. This must be fast, and
 is a clear candidate for assembly optimisation */
static inline tag tag_value16(u_char *buf)
{
  return(char_table_shifted[buf[1]]|char_table[buf[0]]);
}

/* this is like tag_value16 but it uses the unmapped characters */
static inline tag tag_value16u(u_char *buf)
{
  return((buf[1]<<char_bits)|buf[0]);
}

/* check if two characters match */
static inline int u_char_match(u_char c,u_char b)
{
  return(char_table[c] == b);
}

/* compare two strings. We unroll a bit for speed. */
static inline int strnequal(u_char *s1,u_char *s2,int n)
{
  while (n>3) {
    if (!(u_char_match(s2[0],s1[0]) &&
	  u_char_match(s2[1],s1[1]) &&
	  u_char_match(s2[2],s1[2]))) return(0);
    s1 += 3; s2 += 3;
    n -= 3;
  }
  while (n) {
    if (!u_char_match(s2[0],s1[0])) return(0);
    s1++; s2++;
    n--;
  }
  return(1);
}

/* add a tag to the delta table. Possibly update the list of targets
   for the tag as well */
static inline void add_to_table(tag t,int i,int j)
{
  int skip = targets[i].length-(j+tagsize);
  int a;

  delta_table[t] = MIN(delta_table[t],skip);

  if (skip == 0) {
    targets[i].next = MAX(targets[i].next,(int)t);
    tag_table[t] = (tag)i;
  }
}


/* remap the character tables and the targets to pack them into a small
   area in the delta table - this helps the cacheing */
static void remap_char_table(void)
{
  int i,j;
  u_char target_chars[256];

  /* work out what chars exist in the targets */
  memset(target_chars,0,256);
  for (i=0;i<num_targets;i++)
    for (j=0;j<targets[i].length;j++)
      target_chars[targets[i].str[j]] = 1;

  /* force a space to exist */
  if (wsmode) target_chars[SPACE_CHAR] = 1;

  /* map each of the chars to a number */
  j = 1;
  for (i=0;i<256;i++)
    if (target_chars[i]) target_chars[i] = j++;
  max_char = j;

  /* remember what the space is */
  space_char = target_chars[SPACE_CHAR];

  /* remap the targets */
  for (i=0;i<num_targets;i++)
    for (j=0;j<targets[i].length;j++)
      targets[i].str[j] = target_chars[targets[i].str[j]];

  /* remap the char table */
  for (i=0;i<256;i++)
    char_table[i] = target_chars[char_table[i]];

  /* find how many bits are needed for the char table */
  char_bits = 0;
  i = max_char-1;
  while (i) {
    char_bits++;
    i /= 2;
  }

  /* create the shifted table */
  for (i=0;i<256;i++)
    char_table_shifted[i] = char_table[i]<<char_bits;
}


int compare_targets(struct target *t1,struct target *t2)
{
  return(t1->next - t2->next);
}

int compare_targets2(int *t1,int *t2)
{
  return(targets[*t1].next - targets[*t2].next);
}


static void free_old_tables(void)
{
  int i;
  /* free up from the last search */
  for (i=0;i<num_targets;i++) free(targets[i].str);
  if (targets) {free(targets); targets = NULL;}
  if (tag_table) {free(tag_table); tag_table = NULL;}
  if (delta_table) {free(delta_table); delta_table = NULL;}
#if TAB14 || TAB16
  if (delta_table2) {free(delta_table2); delta_table2 = NULL;}
#endif
  num_targets = 0;
}

/* build the tables needed for the bmg search */
int bmg_build_table(u_char *target_string,u_char *char_map,int mode)
{
  u_char *p;
  u_char *tok;
  int i,j;
  int t;
  int dodigraph=0;

  /* remember the wsmode */
  wsmode = mode;

  /* clean up from a previous run */
  free_old_tables();
  space_char = SPACE_CHAR;

  if (!target_string) return(-1);

  if (char_map) {
    for (i=0;i<256;i++)
      char_table[i] = char_map[i];
  } else {
    for (i=0;i<256;i++)
      char_table[i] = isalnum(i)?i:SPACE_CHAR;
  }

  min_target_length = MAX_TARGET_LENGTH;
  max_target_length = 0;

  /* count the targets */
  p = (u_char *)strdup(target_string);
  for (tok=strtok(p,DELIMITERS); tok; tok = strtok(NULL,DELIMITERS))
    num_targets++;
  free(p);

  /* allocate the targets */
  targets = (struct target *)malloc(sizeof(targets[0])*num_targets);

  if (!targets) {
    free_old_tables();
    return(-3);
  }

  /* parse the command */
  p = (u_char *)strdup(target_string);
  if (!p) {
    free_old_tables();
    return(-3);
  }
  i = 0;
  for (tok=strtok(p,DELIMITERS); tok; tok = strtok(NULL,DELIMITERS)) {
    int l = strlen(tok);
    targets[i].next = -1;
    targets[i].str = strdup(tok);
    if (!targets[i].str) {
      free_old_tables();
      return(-3);
    }
    l = MIN(l,MAX_TARGET_LENGTH);
    targets[i].length = l;
    min_target_length = MIN(min_target_length,targets[i].length);
    max_target_length = MAX(max_target_length,targets[i].length);
    if (targets[i].str[l-1]==' ') dodigraph=1; 
    i++;
  }
  free(p);


  /* determine if we will do a digraph search or not */
  if (!dodigraph && num_targets<DIGRAPH_THRESHOLD) {   
    digraph=0;
    tagsize=1;
  } else {
    digraph = 1;
    tagsize = 2;
  }

  if (!num_targets) {
    free_old_tables();
    return(-2);
  }

  remap_char_table();

  /* build the bmg table - this is a mess */
  if (digraph)
    table_size = 1<<(char_bits*2);
  else
    table_size = 256;
  
  delta_table = (u_char *)malloc(table_size*sizeof(delta_table[0]));
  tag_table = (tag *)malloc(table_size*sizeof(tag_table[0]));

  if (!delta_table || !tag_table) {
    free_old_tables();
    return(-3);
  }

#if TAB14 || TAB16
  if (digraph) {
#if TAB14
    int tabsize=(1<<14);
#else
    int tabsize=(1<<16);
#endif
    delta_table2 = (u_char *)malloc(tabsize*sizeof(delta_table[0]));
    if (!delta_table2) {
      free_old_tables();
      return(-3);
    }
  }
#endif

  for (t=0;t<table_size;t++) {
    delta_table[t] = min_target_length;
    tag_table[t] = num_targets;
  }

  if (digraph)
    {
      for (i=0;i<num_targets;i++)
	{
	  int length = targets[i].length;
	  char *str = targets[i].str;
	  u_char c[2];
	  c[1] = str[0];
	  if (length == min_target_length) {
	    if (wsmode) {
	      c[0] = space_char;
	      t = tag_value16u(c);
	      add_to_table(t,i,-1);	    
	    } else {
	      for (c[0]=0;;c[0]++)
		{
		  /* this little loop handles the first char in each
		     target - our tags must match for any possible char
		     to the left of it */
		  t = tag_value16u(c);
		  add_to_table(t,i,-1);
		  if (c[0] == (max_char-1)) break;
		}
	    }
	  }
	  for (j=MAX(0,length-(min_target_length+1));j<=(length-2);j++)
	    {
	      t = tag_value16u(str+j);
	      add_to_table(t,i,j);
	    }
	}
    }
  else
    {
      for (t=0;t<256;t++)
	for (i=0;i<num_targets;i++)
	  {
	    int length = targets[i].length;
	    char *str = targets[i].str;
	    for (j=(length-1);j>=(length-min_target_length);j--)
	      if (char_table[t] == str[j]) {
		add_to_table(t,i,j);
		break; /* go to the next target */
	      }
	  }
    }

  
  if (num_targets>1)
    {
      int *target_map1 = (int *)malloc(sizeof(int)*num_targets);
      int *target_map2 = (int *)malloc(sizeof(int)*num_targets);
      if (!target_map1 || !target_map2) {
	free_old_tables();
	return(-3);
      }
      
      for (i=0;i<num_targets;i++) target_map1[i]=i;
      
      /* sort the targets by the "next" value thay have in the target struct */
      qsort(target_map1,num_targets,sizeof(int),(int (*)())compare_targets2);
      qsort(targets,num_targets,sizeof(targets[0]),(int (*)())compare_targets);

      for (i=0;i<num_targets;i++) target_map2[target_map1[i]]=i;
      
      for (t=0;t<table_size;t++) {    
	if (delta_table[t]) continue;
	tag_table[t] = target_map2[tag_table[t]];
	while (tag_table[t] && 
	       targets[tag_table[t]-1].next == targets[tag_table[t]].next) 
	  tag_table[t]--;
      }
      free(target_map1);
      free(target_map2);
    }
  
  for (i=0;i<num_targets-1;i++)
    if (targets[i].next == targets[i+1].next) 
      targets[i].next = i+1;
    else
      targets[i].next = -1;
  targets[num_targets-1].next = -1;

#if TAB14
  if (digraph) {
    u_char c[2];
    for (c[0]=0;c[0]<(1<<7);c[0]++)
      for (c[1]=0;c[1]<(1<<7);c[1]++)
	delta_table2[TAGV2(c)] = delta_table[tag_value16(c)];
  }
#endif
#if TAB16
  if (digraph) {
    tag v;
    u_char *c = (u_char *)&v;
    for (t=0;t<(1<<16);t++) {
      v = t;
      delta_table2[TAGV2(c)] = delta_table[tag_value16(c)];
    }
  }
#endif

  return(0);
}


/* check if a single proposed match is valid */
static inline int check_target(u_char *buf,int i,void (*fn)())
{
  if (wsmode && char_table[buf[-1]]!=space_char) return(0);
  if (strnequal(targets[i].str,buf,targets[i].length) && fn) {
    fn(buf);
    return(1);
  }
  return(0);
}


/* check if a proposed match is valid against the list of alternates 
   for this tag */
static void check_targets(u_char *buf,tag v,void (*fn)())
{
  int i = tag_table[v];
  do {
    u_char *buf2 = buf-targets[i].length;
    u_char *str = targets[i].str;
    if (u_char_match(*buf2,*str))
      check_target(buf2,i,fn);
    i = targets[i].next;
  } while (i != -1);
}


/* the search proper, 8 bit version - it takes a buffer and finds the
   targets previously specified when building the bmg tables */
#define TAGV(buf) ((buf)[0])
#define DELTAB(buf) delta_table[TAGV(buf)]
#define TAGSIZE 1
static void bmg_search8(u_char *buf,int len,void (*fn)())
{
  u_char *bufend = buf + len - (TAGSIZE-1);
  u_char k;

  buf += min_target_length - TAGSIZE;
  bufend -= 8*MAX_TARGET_LENGTH;

  while (buf<bufend)
    {
      buf += DELTAB(buf);
      buf += DELTAB(buf);
      buf += DELTAB(buf);
      buf += (k = DELTAB(buf));
      if (!k) {
	if (DELTAB(buf-2) < 3 && DELTAB(buf-1) < 2)
	  check_targets(buf+TAGSIZE,TAGV(buf),fn);
	buf++;
      }
    }

  bufend += 8*MAX_TARGET_LENGTH;

  while (buf<bufend) 
    {
      buf += (k = DELTAB(buf));
      if (!k) {
	if (DELTAB(buf-2) < 3 && DELTAB(buf-1) < 2)
	  check_targets(buf+TAGSIZE,TAGV(buf),fn);
	buf++;
      }
    }
}

#undef TAGV
#undef TAGSIZE
#undef DELTAB


#define TAGV(buf) tag_value16(buf)
#if TAB14 || TAB16
#define DELTAB(buf) delta_table2[TAGV2(buf)]
#else
#define DELTAB(buf) delta_table[TAGV(buf)]
#endif
#define TAGSIZE 2
/* this code is identical to the 8 bit search, except for the above two 
   macros  */
static void bmg_search16(u_char *buf,int len,void (*fn)())
{
  u_char *bufend = buf + len - (TAGSIZE-1);
  int k;

  buf += min_target_length - TAGSIZE;
  bufend -= 8*MAX_TARGET_LENGTH;

  while (buf<bufend)
    {
      buf += DELTAB(buf);
      buf += DELTAB(buf);
      buf += DELTAB(buf);
      buf += (k=DELTAB(buf));
      if (!k) {
	if (DELTAB(buf-2) < 3 && DELTAB(buf-1) < 2)
	  check_targets(buf+TAGSIZE,TAGV(buf),fn);
	buf++;
      }
    }

  bufend += 8*MAX_TARGET_LENGTH;

  while (buf<bufend) 
    {
      buf += (k = DELTAB(buf));
      if (!k) {
	if (DELTAB(buf-2) < 3 && DELTAB(buf-1) < 2)
	  check_targets(buf+TAGSIZE,TAGV(buf),fn);
	buf++;
      }
    }
}



/* choose 8 or 16 bit search */
void bmg_search(u_char *buf,int len,void (*fn)())
{
  int i,j;
  if (!num_targets || !buf || !fn || len<1) return;

  /* a special case - check the start of the buffer */
  for (j=0;j<max_target_length;j++) {
    for (i=0;i<num_targets;i++)
      if (len >= targets[i].length && check_target(buf,i,fn)) break;
    buf++; len--;
  }

  if (len < 1) return;

  if (digraph)
    bmg_search16(buf,len,fn);    
  else
    bmg_search8(buf,len,fn);
}
