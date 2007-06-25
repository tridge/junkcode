/*****************************************************************************
A simple C program to detect plagiarism

Idea:

- tokenise input
- collect groups of N tokens, overlapping my M tokens
- hash each group
- sort the resultant hashes

Andrew Tridgell <tridge@samba.org> July 2003

******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

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


/* will I add new words to the word list ? */
BOOL add_words = True;

/* shall I weight the answers ? */
BOOL do_weighting = True;

/* shall I concatenate all adjacent words to form word pairs? */
BOOL concat_words = False;

/* a suspicious filename */
char *suspicious_file = NULL;

/* how to write a boolean */
#define bool_str(v) ((v)?"True":"False")

/* 
a list of all the words found and where they occurred 
*/
struct word
{
  int count; /* how many files this word occurs in */
  char *word; /* text of word */
  BOOL *occurred; /* yes/no has occurred in this file */
} *words = NULL;

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
return a filename
******************************************************************************/
char *FileName(int i)
{
  if (i < num_files)
    return(file_names[i]);
  return(suspicious_file);
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
ask if a word has occurred before - return word index if it has 
return -1 if not
******************************************************************************/
int find_word(char *word)
{
  int ret;
  
  /* we can take advantage of the order of the words */
  ret = insert_position(word);
  if (ret >= num_words || !wordequal(word,words[ret].word))
    return(-1);
  return(ret);

}


/***************************************************************************** 
Insert a word into the list of words that have occurred.
If the word already exists then just set the occured bit for the filenum
******************************************************************************/
void insert_word(char *word,int filenum)
{
  int wordnum,i;
  wordnum = find_word(word);

  if (wordnum < 0 && !add_words)
    return;

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
	(BOOL *)Realloc(NULL,sizeof(BOOL)*(num_files+1));
	
      for (i=0;i<=num_files;i++) words[wordnum].occurred[i] = False;
    }

  if (!words[wordnum].occurred[filenum]) words[wordnum].count++;
  words[wordnum].occurred[filenum] = True;
}

/*****************************************************************************
dump the word occurrance table
******************************************************************************/
void dump_word_table(void)
{
  int i,j;
  for (i=0;i<num_words;i++)
    {
      printf("%20.20s ",words[i].word);
      for (j=0;j<=num_files;j++)
	{
	  if (words[i].occurred[j])
	    printf("1 ");
	  else
	    printf("  ");
	}
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
form the cheating matrix. 
The match between two files is increased if
the word occurs in both files but doesn't occur in more than THRESH % of files.
******************************************************************************/
void cheat_matrix(double threshold1,double threshold2,double output_thresh)
{
  int i,j,wordnum;
  int max_matches1,max_matches2;
  int output_count;

  /* max matches can't be less than 2 */
  if (threshold1 >= 1.0)
    max_matches1 = threshold1;
  else
    max_matches1 = num_files * threshold1;
  if (max_matches1 < 2) max_matches1 = 2;

  if (threshold2 > 1.0)
    max_matches2 = threshold2;
  else
    max_matches2 = num_files * threshold2;
  if (max_matches2 < max_matches1) max_matches2 = max_matches1;


  if (output_thresh >= 1.0)
    output_count = output_thresh;
  else
    output_count = output_thresh * num_files;
  if (output_count < 1) output_count = 1;
  if (output_count > num_files) output_count = num_files;
  

#define CMAT(i,j) mat[(i)*(num_files+1) + j]

  /* alloc the cheat matrix */
  mat = (struct pair *)Realloc(mat,(num_files+1)*(num_files+1)*sizeof(*mat));

  /* reset it */
  for (i=0;i<=num_files;i++)
    for (j=0;j<=num_files;j++)
      {
	CMAT(i,j).i = i; CMAT(i,j).j = j;
	CMAT(i,j).count=0;
      }

  /* process the words one at a time */
  for (wordnum=0;wordnum<num_words;wordnum++)
    {
      int occurrances = words[wordnum].count;

      /* if the word is very common then forget it */
      if (occurrances > max_matches1)
	{
	  if (debug > 3)
	    printf("ignoring common word %s (%d occurrances)\n",
		   words[wordnum].word,occurrances);

	  if (occurrances > max_matches2)
	    words[wordnum].count = 0;

	  continue;
	}      

      if (debug > 3)
	printf("%s occurred %d times\n",words[wordnum].word,occurrances);

      /* increment possible cheaters */
      for (i=0;i<=num_files;i++)
	{
	  if (words[wordnum].occurred[i])
	    for (j=i+1;j<=num_files;j++)
		    if (words[wordnum].occurred[j]) {
			    if (do_weighting) {
				    CMAT(i,j).count += ((max_matches1+1) - words[wordnum].count);
			    }	else {
				    CMAT(i,j).count++;
			    }
		    }
	}            
    }

  /* sort them */
  qsort(mat,(num_files+1)*(num_files+1),sizeof(*mat),(int (*)())pair_compare);

  /* sort the wordlist so least frequent words are at the top */
  qsort(words,num_words,sizeof(*words),(int (*)())words_compare);

  /* find the highest */
  {
    int f;
    for (f=0;f<output_count;f++)
      if (mat[f].count > 0)
	{
	  i = mat[f].i; j = mat[f].j;
	  printf("scored %3d in (%s %s)\n",mat[f].count,
		 FileName(i),
		 FileName(j));
	
	  for (wordnum=0;wordnum<num_words;wordnum++)
	    {
	      if (words[wordnum].count>0 &&
		  words[wordnum].occurred[i] && 
		  words[wordnum].occurred[j])
		printf("%s(%d) ",words[wordnum].word,words[wordnum].count);
	    }            
	  printf("\n\n");
	}
  }
}


/*****************************************************************************
process one file
******************************************************************************/
void process_one(char *filename,char *filter_prog,int filenum)
{
  FILE *f;
  char *word;
  static char lastword[MAX_WORD_LEN+1];

  if (filter_prog)
    {
      char cmd[1000];
      sprintf(cmd,"cat %s | %s",filename,filter_prog);
      f = popen(cmd,"r");
    }
  else
    f = fopen(filename,"r");

  if (!f) 
    {
      perror(filename);
      return;
    }

  if (debug > 0)
    printf("processing file %s\n",filename);

  fflush(stdout);

  while ((word = get_word(f)) != NULL) {
	  insert_word(word,filenum);
	  if (concat_words) {
		  strcat(lastword, word);
		  insert_word(lastword,filenum);
		  strcpy(lastword, word);
	  }
  }
  if (filter_prog)
    pclose(f);
  else
    fclose(f);
}


/*****************************************************************************
process the files by opening them and getting the words
******************************************************************************/
void process_files(char *filter_prog)
{
  int i;

  if (suspicious_file)
    {
      process_one(suspicious_file,filter_prog,num_files);
      add_words = False;
    }

  for (i=0;i<num_files;i++)
    {
      if (suspicious_file && strcmp(suspicious_file,FileName(i))==0)
	continue;

      printf(".");
      process_one(FileName(i),filter_prog,i);
    }

  printf("\nfinished initial processing\n");
}

/*****************************************************************************
usage of the program
******************************************************************************/
void usage(void)
{
  printf("detect: find cheaters by looking at token similarities between files\n");
  printf("\ndetect [options] <files..>\n");
  printf("\t-h = give extended help and examples of use (RECOMMENDED)\n");
  printf("\t-c = be case sensitive\n");
  printf("\t-n num = output the top n pairs (default top 20%%)\n");
  printf("\t-t thresh = set threshold1 (default 3)\n");
  printf("\t-T thresh = set threshold2 (default 10)\n");
  printf("\t-C concatenate adjacent tokens to make new tokens (very useful!)\n");
  printf("\t-I ignore_str = ignore these chars in the file\n");
  printf("\t-f prog = filter the files through the given program\n");
  printf("\t-s filename = only look for matches to this particular suspicious file\n");
  printf("\t-w toggle weighting of the score by (threshold1-frequency) (default True)\n");
  printf("\n");
}

/*****************************************************************************
provide extended help
******************************************************************************/
void help(void)
{
  /* NOTE: this bit of text relies on a compiler that can handle new */
  /* lines in string literals. gcc is good at this */

  char *help_txt = "
This program tries to identify common words between a list of files
in an attempt to find cases of plagiarism. 

Algorithm:
==========

1) Build a list words that occur in the files, and a bitmap of which
   words occur in which files.
2) Discard any words that occur in more than <threshold2> files
3) Produce a matrix M where M(i,j) is the number of times a word was
   used both in file i and file j, not counting words that occur in
   more than <threshold1> files.
4) Sort this matrix to produce the top <n> pairs, weighting by
   how infrequent the words are.
5) Write out the pairs along with what words matched between them and
   the frequency of those words.


Interpreting the output:
========================

Here is some sample output.

scored  13 in (stu1.mod,stu2.mod)
AveAss(2) CalculateAss(2) OutputResult(2) StuMark(2) stuNum(2)
InputFile(3) MaxAss(3) ReadAss(3) index(4) TutTotal(5)

This means that these two files (stu1.mod and stu2.mod) both contained
all the identifiers listed. The identifiers with a (2) after them
occurred only in these two files. The identifier TutTotal was used in
5 files and is therefore less suspicious.

Example 1:
==========

   detect *.mod

This will find common tokens between a list of modula files, listing
which files have lots of common tokens, what the common tokens are and
how common those tokens are across all the files.

Example 2:
==========

  detect -c -n 10 -t 2 -T 20 -I ',_ ' *.c

This will look in a bunch of C progs, and rank the pairs that used
tokens that were used by only 2 files. It will list common tokens
between the files up to those tokens used by 20 files. It will skip
over all underscores, commas and spaces. This last step is useful to
catch people who just inserted underscores. Skipping spaces and commas
has the effect of treating whole argument lists as a single word which
can be a very effective technique. This example will also be case
sensitive in comparisons. It will only show the top 10 matching pairs.

Example 3:
==========

  detect -f 'grep PROCEDURE' *.mod

This will only look at lines that have the word PROCEDURE in them.
This can be good for checking only whether procedure names match.


Example 4:
==========
 
   detect -s stu1.mod *.mod

This will only look for matches between the 'suspicious file' stu1.mod
and all the other files. This is useful to detect cheating where you
suspect a particular person, or to find a group who collaborated once
you find one pair that are a good match.

Example 5:
==========
 
  detect -w *.mod

This will disable weighting of the scores by (threshold1-frequency)
thus making all matches of equal weight.

Example 6:
==========

  detect -C `find . -name \"*.txt\"`

This will process all text files below the current directory and will
also enable token concatenation. Token concatenation is very useful as
it builds a much larger token pool with much less common tokens. It is
particularly useful for essay type assignments which lack unique
programming identifiers.

Advanced Notes:
===============

*) The filter programs used by -f can be any program that reads from
   standard input and writes to standard output. 

*) The two thesholds and the output count can be given either as a
   proportion of the total number of files or as a absolute count. Thus
   `-t 0.05' will set the first threshold to 5% of the total number of
   files. Any number less than 1 is interpreted as a proportion.


Author:
=======

 Andrew Tridgell
 Andrew.Tridgell@anu.edu.au

";


  usage();
  puts(help_txt);

}



/*****************************************************************************
the main program 
******************************************************************************/
int main(int argc,char **argv)
{
  double threshold1 = 3;
  double threshold2 = 10;
  double output_thresh = 0.2;
  char *filter_prog=NULL;

  int opt;
  extern char *optarg;
  extern int optind;

  /* get some options */
  while ((opt = getopt (argc, argv, "Cwct:T:I:d:hbf:n:s:")) != EOF)
    switch (opt)
      {
      case 'n':
	output_thresh = atof(optarg);
	break;
      case 'f':
	filter_prog = strdup(optarg);
	break;
      case 'h':
	help();
	exit(0);
      case 'c':
	case_sensitive = !case_sensitive;
	break;
      case 'C':
	concat_words = !concat_words;
	break;
      case 'w':
	do_weighting = !do_weighting;
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
      case 's':
	suspicious_file = strdup(optarg);
	break;
      default:
	usage();	
	exit(0);	
      }

  /* get the file names */
  num_files = argc-optind;
  file_names = &argv[optind];

  if (num_files < 2)
    {
      usage();
      exit(0);
    }

  printf("\nProcessing %d files\n",num_files);

  if (suspicious_file)
    printf("Suspicious file = %s\n",suspicious_file);

  printf("Weighting=%s\tCase Sensitive=%s\n",
	 bool_str(do_weighting),bool_str(case_sensitive));

  printf("Threshold1=%g\tThreshold2=%g\n",threshold1,threshold2);

  if (*ignore_chars)
    printf("Ignoring chars `%s'\n",ignore_chars);

  if (filter_prog)
    printf("Filtering with `%s'\n",filter_prog);

  /* process the files */
  process_files(filter_prog);

  if (debug > 2)
    dump_word_table();

  /* and get the matches */
  cheat_matrix(threshold1,threshold2,output_thresh);

  return 0;
}
