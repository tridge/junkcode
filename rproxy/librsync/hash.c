#include "includes.h"

extern int csum_length;

extern int verbose;
extern int am_server;

extern int remote_version;

typedef unsigned short tag;

#define TABLESIZE (1<<16)
#define NULL_TAG (-1)


extern struct stats stats;

struct target {
  tag t;
  int i;
};

static struct target *targets;

static int *tag_table;

#define gettag2(s1,s2) (((s1) + (s2)) & 0xFFFF)
#define gettag(sum) gettag2((sum)&0xFFFF,(sum)>>16)

static int compare_targets(struct target *t1,struct target *t2)
{
  return((int)t1->t - (int)t2->t);
}


static struct sum_struct * sigs;

int
build_hash_table(struct sum_struct *s)
{
  int i;

  sigs = s;

  if (!tag_table)
    tag_table = (int *)malloc(sizeof(tag_table[0])*TABLESIZE);

  targets = (struct target *)malloc(sizeof(targets[0])*s->count);
  if (!tag_table || !targets) {
	  errno = ENOMEM;
	  return -1;
  }

  for (i=0;i<s->count;i++) {
    targets[i].i = i;
    targets[i].t = gettag(s->sums[i].sum1);
  }

  qsort(targets,s->count,sizeof(targets[0]),(int (*)())compare_targets);

  for (i=0;i<TABLESIZE;i++)
    tag_table[i] = NULL_TAG;

  for (i=s->count-1;i>=0;i--) {    
    tag_table[targets[i].t] = i;
  }

  return 0;
}


int
find_in_hash(int sum,char *(sum_fun)(void)) {
  char * sum2 = NULL;
  int j = tag_table[gettag(sum)];

  if (j == NULL_TAG) {
    return -1;
  }
  
  for (; j<sigs->count && targets[j].t == gettag(sum); j++) {
    int i = targets[j].i;
    
    if (sum != sigs->sums[i].sum1) continue;
    
    /* also make sure the two blocks are the same length */
/*      l = MIN(s->n,len-offset); */
/*      if (l != s->sums[i].len) continue;			 */
    
/*      if (!done_csum2) { */
/*        map = (schar *)map_ptr(buf,offset,l); */
/*        get_checksum2((char *)map,l,sum2); */
/*        done_csum2 = 1; */
/*      } */

    sum2 = sum_fun();
    if (!sum2) {
      fprintf(stderr, "sum2 undefined\n");
      exit(0);
    }
    
    if (memcmp(sum2,sigs->sums[i].sum2,SUM_LENGTH) == 0) {
      return sigs->sums[i].i;
    }    
  }

  return -1;
}

