/* written by Andrew Tridgell (May 1992) */


#include <stdio.h>
#include <malloc.h>
#include <varargs.h>
#include <stdlib.h>
#include <time.h>

int xsize=0;
int ysize=0;

char **grid;
char *wordfile;
char **wordlist=NULL;
int num_words=0;

enum _Direction{ACROSS,DOWN};
typedef enum _Direction Direction; 
typedef int BOOL;
#define True 1
#define False 0
#define CONST const
#define LONG_STRING_LENGTH 200

char *strtidy();
void read_a_line();
char *my_fgets();


#ifndef MSDOS
#define randomize() srand(time(NULL))
#define ctrlbrk(fn)
#define BLACKSQUARE ' '
#else
#define BLACKSQUARE 'Û'
#endif

/*******************************************************************
create a matrix of any dimension. The return must be cast correctly.
********************************************************************/
void *any_matrix(va_alist)
va_dcl
{
int dimension;
int el_size;
int *dims=NULL;
void **mat;
int i,j,size,ptr_size,ppos,prod;
int padding;
void *next_ptr;
va_list ap;

/* first gather the arguments */
va_start(ap);
dimension = va_arg(ap, int);
el_size = va_arg(ap, int);


if (dimension <= 0) return(NULL);
if (el_size <= 0) return(NULL);

dims = (int *)malloc(dimension * sizeof(int));
if (dims == NULL) return(NULL);
for (i=0;i<dimension;i++)
	dims[i] = va_arg(ap, int);
va_end(ap);

/* now we've disected the arguments we can go about the real business of
creating the matrix */

/* calculate how much space all the pointers will take up */
ptr_size = 0;
for (i=0;i<(dimension-1);i++)
	{
	prod=sizeof(void *);
	for (j=0;j<=i;j++) prod *= dims[j];
	ptr_size += prod;
	}

/* padding overcomes potential alignment errors */
padding = (el_size - (ptr_size % el_size)) % el_size;

/* now calculate the total memory taken by the array */
{
prod=el_size;
for (i=0;i<dimension;i++) prod *= dims[i];
size = prod + ptr_size + padding;
}

/* allocate the matrix memory */
mat = (void **)malloc(size);

if (mat == NULL)
	{
	fprintf(stdout,"Error allocating %d dim matrix of size %d\n",dimension,size);
	free(dims);
	return(NULL);
	}

/* now fill in the pointer values */
next_ptr = (void *)&mat[dims[0]];
ppos = 0;
prod = 1;
for (i=0;i<(dimension-1);i++)
{
int skip;
if (i == dimension-2) 
  {
    skip = el_size*dims[i+1];
    next_ptr = (void *)(((char *)next_ptr) + padding); /* add in the padding */
  }
else
  skip = sizeof(void *)*dims[i+1];

for (j=0;j<(dims[i]*prod);j++)
  {
    mat[ppos++] = next_ptr;
    next_ptr = (void *)(((char *)next_ptr) + skip);
  }
prod *= dims[i];
}

free(dims);
return((void *)mat);
}


/*******************************************************************
return a random number
********************************************************************/
int random_num(void)
{
return(rand());
}


/*******************************************************************
this returns the number of lines in a text file
********************************************************************/
int num_text_lines(CONST char *fname)
{
FILE *file;
int count = 0;
char buf[2000];

file = fopen(fname,"r");
if (!file)
  return(0);

while (!feof(file))
  {
    read_a_line(buf,2000,file);
    if (*buf) count++;
  }

fclose(file);
return(count);
}

/*******************************************************************
read a line from a file. If the line is of 0 length then read another
********************************************************************/
void read_a_line(char *buf,int maxlen,FILE *file)
{
my_fgets(buf,maxlen,file);
if (strlen(buf) == 0)
	my_fgets(buf,maxlen,file);
}

/*******************************************************************
like fgets but remove trailing CR or LF
********************************************************************/
char *my_fgets(char *s,int n,FILE *stream)
{
char *ret;

ret = fgets(s,n,stream);
if (ret == NULL) 
  {
    *s = 0;
    return(NULL);
  }

return(strtidy(s,"\n\r "));
}

/*******************************************************************
remove specified chars from front and back of a string 
********************************************************************/
char *strtidy(char *str,CONST char *chars)
{
int len=strlen(str);
while ((len > 0) && (strchr(chars,*str) != NULL))
	{
	memcpy(str,&str[1],len);
	len--;
	}
while ((len > 0) && (strchr(chars,str[len-1]) != NULL))
	{
	str[len-1]=0;
	len--;
	}
return(str);
}




/*******************************************************************
load a list of words
********************************************************************/
char **load_word_list(char *fname,int *num)
{
FILE *file;
int i;
char line[LONG_STRING_LENGTH];
char **list;
*num = num_text_lines(fname);
if (*num < 1)
  return(NULL);

list = (char **)malloc(sizeof(char *)*(*num));

file = fopen(fname,"r");
for (i=0;i<(*num);i++)
  {
    read_a_line(line,LONG_STRING_LENGTH,file);
    list[i] = (char *)malloc(strlen(line)+1);
    strcpy(list[i],line);
  }
fclose(file);

return(list);
}

/*******************************************************************
place a word
********************************************************************/
void PlaceWord(char *word,int i,int j,Direction dir)
{
int k;
int len=strlen(word);
if (dir == ACROSS)
  {
    for (k=0;k<len;k++)
      grid[i+k][j] = word[k];
  }
else
  {
    for (k=0;k<len;k++)
      grid[i][j+k] = word[k];
  }
}

/*******************************************************************
determine if a word is legal in a position
********************************************************************/
BOOL Legal(char *word,int i,int j,Direction dir)
{
int len=strlen(word);
if (dir == ACROSS)
  {
    int k;
    if (i+len > xsize) return(False);
    if ((i != 0) && grid[i-1][j]) return(False);
    if (((i+len) != xsize) && grid[i+len][j]) return(False);
    for (k=0;k<len;k++)
      if (grid[i+k][j] && (grid[i+k][j] != word[k])) return(False);
    for (k=0;k<len;k++)
      {
	if ((j != 0) && grid[i+k][j-1] && !grid[i+k][j]) return(False);
	if ((j != (ysize-1)) && grid[i+k][j+1] && !grid[i+k][j]) return(False);
      }
  }
else
  {
    int k;
    if (j+len > ysize) return(False);
    if ((j != 0) && grid[i][j-1]) return(False);
    if (((j+len) != ysize) && grid[i][j+len]) return(False);
    for (k=0;k<len;k++)
      if (grid[i][j+k] && (grid[i][j+k] != word[k])) return(False);
    for (k=0;k<len;k++)
      {
	if ((i != 0) && grid[i-1][j+k] && !grid[i][j+k]) return(False);
	if ((i != (xsize-1)) && grid[i+1][j+k] && !grid[i][j+k]) return(False);
      }
  }
return(True);
}

/*******************************************************************
score a word in a position
********************************************************************/
int Score(char *word,int i,int j,Direction dir)
{
int len=strlen(word);
int score=0;
if (dir == ACROSS)
  {
    int k;
    for (k=0;k<len;k++)
      if (grid[i+k][j])
	{
	  if ((k == 0) || (k == (len-1))) 
	    score += 2;
	  else
	    score += 3;
	}
    if ((j != 0) && (j != (ysize-1))) score++;
  }
else
  {
    int k;
    for (k=0;k<len;k++)
      if (grid[i][j+k])
	{
	  if ((k == 0) || (k == (len-1))) 
	    score += 4;
	  else
	    score += 6;
	}
    if ((i != 0) && (i != (xsize-1))) score++;
  }
return(score);
}

Direction last_dir=ACROSS;


/*******************************************************************
find the best position for a word
********************************************************************/
BOOL BestPosition(char *word,int *besti,int *bestj,Direction *dir)
{
int best;
int i,j;
Direction d;
best = -1;
for (i=0;i<xsize;i++)
  for (j=0;j<ysize;j++)
    {
      int s;
      d = ACROSS;
      if (Legal(word,i,j,d))
	{
	  s = Score(word,i,j,d);
	  if (last_dir != d) s++;
	  if (s > best || ((s == best) && ((random_num()%(xsize*ysize/4))!=0)))
	    {
	      best = s;
	      *besti = i;
	      *bestj = j;
	      *dir = d;
	    }
	}
      d = DOWN;
      if (Legal(word,i,j,d))
	{
	  s = Score(word,i,j,d);
	  if (last_dir != d) s++;
	  if (s > best || ((s == best) && ((random_num()%(xsize*ysize/4))!=0)))
	    {
	      best = s;
	      *besti = i;
	      *bestj = j;
	      *dir = d;
	    }
	}
    }
return(best >= 0);
}

/*******************************************************************
zero a crossword
********************************************************************/
void zero_crossword(void)
{
int i,j;
for (i=0;i<xsize;i++)
  for (j=0;j<ysize;j++)
    grid[i][j] = 0;
}


/*******************************************************************
build a crossword
********************************************************************/
int BuildCrossword(char **list,int num)
{
int i,j;
Direction d;
int remaining=num;
int bad=0;
BOOL *used = (BOOL *)malloc(sizeof(BOOL)*num);
for (i=0;i<num;i++)
  used[i] = False;
zero_crossword();
while (remaining > 0)
  {
    int choose=-1;
    while (choose==-1)
      {
	choose = random_num() % num;
	if (used[choose]) choose=-1;
      }
    used[choose] = True;
    remaining--;
    if (BestPosition(list[choose],&i,&j,&d))
      PlaceWord(list[choose],i,j,d);
    else
      bad++;
  }
return(num-bad);
}

/*******************************************************************
build a crossword
********************************************************************/
int BuildBestCrossword(char **list,int num)
{
int i,j;
Direction d;
int remaining=num;
int bad=0;
BOOL *used = (BOOL *)malloc(sizeof(BOOL)*num);
int *scores = (int *)malloc(sizeof(int)*num);

for (i=0;i<num;i++)
  used[i] = False;

zero_crossword();
while (remaining > 0)
  {
    int n;
    int choose;
    for (i=0;i<num;i++)
      scores[i] = -1;
    for (n=0;n<num;n++)
      if (!used[n] && BestPosition(list[n],&i,&j,&d))
	scores[n] = Score(list[n],i,j,d);
    {
      int numbest=0,bestscore=scores[0];
      int k;
      for (n=0;n<num;n++)
	{
	  if (scores[n] == bestscore) numbest++;
	  if (scores[n] > bestscore)
	    {
	      bestscore = scores[n];
	      numbest = 1;
	    }
	}
      if (bestscore < 0) return(num-remaining);
      k = random_num() % numbest;
      numbest=0;
      for (n=0;n<num;n++)
	{
	  if (scores[n] == bestscore) 
	    {
	      if (numbest == k) choose=n;
	      numbest++;
	    }
	}
    }
    BestPosition(list[choose],&i,&j,&d);
    PlaceWord(list[choose],i,j,d);
    used[choose] = True;
    remaining--;
  }
return(num-remaining);
}

/*******************************************************************
display the crossword
********************************************************************/
void DisplayCrossword(FILE *f)
{
int i,j;
for (j=0;j<ysize;j++)
  {
    for (i=0;i<xsize;i++)
      {
	if (grid[i][j])
	  fputc(grid[i][j],f);
	else
	  fputc(BLACKSQUARE,f);
      }
    fputc('\n',f);
  }
putchar('\n');
}


/*******************************************************************
save the crossword in a pzl file
********************************************************************/
void SavePuzzle(char *fname)
{
FILE *f = fopen(fname,"w");
int i,j;
if (!f) return;

fprintf(f,"%cXWORD0%s%c%c%2d%2d",42,"Sue2",3,3,xsize,ysize);

for (j=0;j<ysize;j++)
    for (i=0;i<xsize;i++)
      fprintf(f,"%c %c",(grid[i][j]?grid[i][j]:' '),3);

fclose(f);
}


/*******************************************************************
save the crossword in a .cwd file (for ccwin15.zip)
********************************************************************/
void SaveCross(char *fname)
{
FILE *f = fopen(fname,"w");
int i,j;
char head[10];

if (!f) return;

memset(head,0,10);
head[0] = 'j';
head[4] = '\n';
head[6] = 1;
head[8] = xsize;
head[9] = ysize;

fwrite(head,1,10,f);

for (j=0;j<ysize;j++)
  for (i=0;i<xsize;i++)
    fprintf(f,"%c",grid[i][j]?(char)toupper(grid[i][j]):0333);

head[0] = 0;
head[1] = 0;

fwrite(head,1,2,f);

fclose(f);
}


void copy_to(char **g)
{
int i,j;
for (i=0;i<xsize;i++)
for (j=0;j<ysize;j++)
	g[i][j] = grid[i][j];
}

int cbreak(void)
{
return(0);
}

int main(int argc,char *argv[])
{
char **bestgrid;
int best=-1;
ctrlbrk(cbreak);

randomize();
if (argc < 4)
  {
    printf("angry: Xsize Ysize WordFile\n");
    return(0);
  }
xsize = atoi(argv[1]);
ysize = atoi(argv[2]);
wordfile = argv[3];

grid = (char **)any_matrix(2,sizeof(char),xsize,ysize);
bestgrid = (char **)any_matrix(2,sizeof(char),xsize,ysize);
if (!grid || !bestgrid)
  {
    printf("invalid xsize or ysize\n");
    return(0);
  }

wordlist = load_word_list(wordfile,&num_words);
while (True)
  {
  int n;
    n = BuildBestCrossword(wordlist,num_words);
    putchar('.');
    if (n > best)
	{
	best = n;
	copy_to(bestgrid);
	printf("\nplaced %d words\n",best);
	{
	FILE *f = fopen("best.doc","w");
	DisplayCrossword(stdout);
	DisplayCrossword(f);
	SavePuzzle("best.pzl");
	SaveCross("best.cwd");
	fclose(f);
	}
	}
    fflush(stdout);
  }
}
