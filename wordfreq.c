/*****************************************************************************
A simple C program to detect cheating

Idea:

- create a list of words in the files
- foreach word create a bitmap of where they occurred 
- ignore words that occur in a large proportion of files
- produce a matrix of co-occurances
- output the pairs with the highest co-occurances


Enhancements:
- two thresholds, one for rating matches, one for display of matches
- ignore strings; allowing certain chars to be skipped in the input

Andrew Tridgell, May 1994

******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef enum {False=0,True=1} BOOL;

/* experimental ? */
BOOL beta = False;

/* should I be case sensitive? */
BOOL case_sensitive = False;

/* how big can a word get? */
#define MAX_WORD_LEN 500

/* how many files are we processing? */
int num_files = 0;

/* the names of the files we are processing - normally stolen from argv */
char **file_names = NULL;

/* how many words have we found? */
int num_words = 0;

/* a list of chars to ignore - can be interesting */
char ignore_chars[MAX_WORD_LEN] = "";

/* what debug level? */
int debug = 0;

/* 
a list of all the words found and where they occurred 
*/
struct word
{
  int count; /* how many files this word occurs in */
  char *word; /* text of word */
  int *occurred; /* count of occurrance in each file */
} *words = NULL;

int *file_totals=NULL;

/***************************************************************************** 
like realloc() but can start from scratch, and exits if there is an error
******************************************************************************/
void *Realloc(void *ptr,int size)
{
  if (!ptr)
    ptr = malloc(size);
  else
    ptr = realloc(ptr,size);
  if (!ptr)
    {
      printf("Can't allocate %d bytes\n",size);
      exit(0);
    }
  return(ptr);
}

/*****************************************************************************
decide if two words are equal
******************************************************************************/
BOOL wordequal(char *word1,char *word2)
{
  if (case_sensitive)
    return(strcmp(word1,word2) == 0);
  else
    return(strcasecmp(word1,word2) == 0);
}


/***************************************************************************** 
ask if a word has occurred before - return word index if it has 
return -1 if not
******************************************************************************/
int find_word(char *word)
{
  int i;
  int ret;
  
  /* we can take advantage of the order of the words */
  ret = insert_position(word);
  if (ret >= num_words || !wordequal(word,words[ret].word))
    return(-1);
  return(ret);

}

/***************************************************************************** 
find an insertion position for a new word
******************************************************************************/
int insert_position(char *word)
{
  int high_i = num_words;
  int low_i = 0;
  int guess,ret;

  /* do a bisection search - this assumes the list is kept ordered */
  while (high_i > low_i)
    {
      guess = (high_i + low_i)/2;
      if (case_sensitive)
	ret = strcmp(word,words[guess].word);
      else
	ret = strcasecmp(word,words[guess].word);
      if (ret == 0)
	return(guess);
      if (ret > 0)
	low_i = guess+1;
      if (ret < 0)
	high_i = guess;
    }
  return(low_i);
}


/***************************************************************************** 
Insert a word into the list of words that have occurred.
If the word already exists then just set the occured bit for the filenum
******************************************************************************/
void insert_word(char *word,int filenum)
{
  int wordnum,i;
  wordnum = find_word(word);
  if (wordnum < 0)
    {
      if (debug > 0)
	printf("new word %s from filenum %d\n",word,filenum);

      wordnum = insert_position(word);
      words = (struct word *)Realloc(words,sizeof(struct word)*(num_words+1));
      num_words++;

      for (i=num_words-2;i>=wordnum;i--)
	words[i+1] = words[i];

      words[wordnum].count=0;
      words[wordnum].word = strdup(word);
      words[wordnum].occurred = 
	(int *)Realloc(NULL,sizeof(int)*num_files);
	
      for (i=0;i<num_files;i++) words[wordnum].occurred[i] = 0;
    }

  if (!words[wordnum].occurred[filenum]) words[wordnum].count++;
  words[wordnum].occurred[filenum]++;
}

/*****************************************************************************
dump the word occurrance table
******************************************************************************/
void dump_word_table(void)
{
  int i,j;
  for (i=0;i<num_files;i++)
    printf("%s has %d words\n",file_names[i],file_totals[i]);
  printf("\n");

  for (i=0;i<num_words;i++)
    {
      printf("%-20.20s\t",words[i].word);
      for (j=0;j<num_files;j++)
	printf("%6.2f (%5d)   \t",
	       1000.0*100.0*words[i].occurred[j]/file_totals[j],
	       words[i].occurred[j]);
      printf("\n");
    }
}

/*****************************************************************************
read one character - skipping over the chars in ignore
******************************************************************************/
int Read(FILE *f,char *ignore)
{
  int ch = fgetc(f);
  if (ch == EOF) return(EOF);
  if (*ignore && (strchr(ignore,(char)ch) != NULL))
    return(Read(f,ignore));
  return(ch);
}

/*****************************************************************************
get a word from a stream. return the word. return NULL if no more words 
exist. possibly skip comments?
******************************************************************************/
char *get_word(FILE *f)
{
  static char word[MAX_WORD_LEN+1];
  int word_len = 0;
  char ch;

  strcpy(word,"");

  /* skip over non alphanum chars */
  while ((ch = Read(f,ignore_chars)) != EOF)
    {
      if (isalnum(ch)) break;
    }
  if (ch == EOF) return(NULL);

  word[word_len++] = ch;
  
  /* grab all the alphanums */
  while (word_len < MAX_WORD_LEN && (isalnum(ch = Read(f,ignore_chars))))
    word[word_len++] = ch;

  word[word_len] = '\0';
  return(word);
}


/* used for cheating matrix */
struct pair
{
  int i,j;
  int count;
} *mat = NULL;

/***************************************************************************** 
   compare 2 pair structures - used for sorting 
******************************************************************************/
int pair_compare(struct pair *t1,struct pair *t2)
{
  return(t2->count - t1->count);
}

/***************************************************************************** 
   compare 2 word structures - used for sorting 
******************************************************************************/
int words_compare(struct word *t1,struct word *t2)
{
  if (t2->count == t1->count)
    return(strcmp(t1->word,t2->word));
  return(t1->count - t2->count);
}


/*****************************************************************************
process the files by opening them and getting the words
******************************************************************************/
void process_files(void)
{
  int i;

  file_totals = (int *)malloc(sizeof(int)*num_files);
  bzero(file_totals,sizeof(int)*num_files);

  for (i=0;i<num_files;i++)
    {
      FILE *f = fopen(file_names[i],"r");
      char *word;
      if (!f) 
	{
	  printf("Can't open %s\n",file_names[i]);
	  continue;
	}
#if 0
      printf("processing file %s\n",file_names[i]);
      printf(".");
#endif
      fflush(stdout);
      while ((word = get_word(f)) != NULL) {
	insert_word(word,i);
	file_totals[i]++;
      }
      fclose(f);
    }
  printf("\n");
}

/*****************************************************************************
provide extended help
******************************************************************************/
void help(void)
{
#if 0
  char *txt = 

This program tries to identify common words between a list of files,
in an attempt to find cases of plagiarism. It first builds a list of
all words in all the files, and a bitmap of which words occur in which
files. It then ignores very common words

#endif
  printf("extended help is not written yet - are you volunteering?\n\n");
  exit(0);
}

/*****************************************************************************
usage of the program
******************************************************************************/
void usage(void)
{
  printf("detect: find cheaters by looking at token similarities between files\n");
  printf("\ndetect [options] <files..>\n");
  printf("\t-c = be case sensitive\n");
  printf("\t-t thresh = set threshold1\n");
  printf("\t-T thresh = set threshold2\n");
  printf("\t-I ignore_str = ignore these chars in the file\n");
  printf("\n");
  exit(0);
}


/*****************************************************************************
the main program 
******************************************************************************/
int main(int argc,char **argv)
{
  double threshold1 = 0.01;
  double threshold2 = 0.1;

  int opt;
  extern char *optarg;
  extern int optind;

  /* get some options */
  while ((opt = getopt (argc, argv, "ct:T:I:d:hb")) != EOF)
    switch (opt)
      {
      case 'h':
	help();
	break;
      case 'c':
	case_sensitive = !case_sensitive;
	break;
      case 'b':
	beta = !beta;
	break;
      case 't':
	threshold1 = atof(optarg);
	break;
      case 'T':
	threshold2 = atof(optarg);
	break;
      case 'I':
	strcat(ignore_chars,optarg);
	break;
      case 'd':
	debug = atoi(optarg);
	break;
      default:
	usage();	
      }

  /* get the file names */
  num_files = argc-optind;
  file_names = &argv[optind];

  if (num_files < 2)
    usage();

  /* process the files */
  process_files();

  dump_word_table();
}
