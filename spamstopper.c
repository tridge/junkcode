/* a simple program to detect if an email arriving on standard input is SPAM */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>


/* a single occurance of any of these strings gets it banned */
static char *spam_strings[] = {
"Extractor Pro",
"HUGE PROFITS",
"removed from this advertiser",
"Adult Entertainment",
"PAY PLAN",
"FREE REPORT",
"RED HOT",
"$$$",
"MoneyMaker",
"cash register",
"build your business",
"having sex",
"home video",
"fraction of the price",
"ALL PRICES",
"Game List",
"electron28",
" rape ",
" sex ",
"secrets of success",
"attractive woman",
"love life",
"beautiful women",
"INTRODUCTORY OFFER",
"ORDER IT NOW",
"CREDIT CARD",
"Money Making",
"FREE TAPE",
"Network Marketing",
"HOME BUSINESS",
"Cold Cash",
"Adult Entertainment",
"COMMISSIONS",
"TOP PRICE",
"DON'T BELIEVE THEM",
"BELIEVE ME",
"HERE'S THE PROOF",
"SIMPLE DETAILS",
"DOLLAR BILL",
"MAKE MONEY",
NULL};



/* SPAM_WORDS_LIMIT occurances of any of these strings gets it banned */
#define SPAM_WORDS_LIMIT 2
static char *spam_words[] = {
  "profit",
  "Profit",
  "PROFIT",

  "cash",
  "Cash",
  "CASH",

  "opportunity",
  "Opportunity",
  "OPPORTUNITY",

  "money",
  "Money",
  "MONEY",

  "Coming soon",

  "business",

  "video",

  "marketing",

  NULL};


static int match_spam_strings(char *buf, int size)
{
  int i;

  for (i=0;spam_strings[i];i++) {
    if (memmem(buf, size, spam_strings[i], strlen(spam_strings[i])))
      return 1;
  }

  return 0;
}


static int match_spam_words(char *buf, int size)
{
  int i;
  int count=0;

  for (i=0;spam_words[i];i++) {
    if (memmem(buf, size, spam_words[i], strlen(spam_words[i]))) {
      count++;
    }
  }

  if (count >= SPAM_WORDS_LIMIT)
    return 1;

  return 0;
}


/* messages longer than this get truncated */
#define MAXBUF 0xFFFF

int main(int argc, char *argv[])
{
  char *buf;
  int size;

  if (argc != 2) {
    fprintf(stderr,"spamstopper <spamfile>\n");
    exit(1);
  }

  buf = (char *)malloc(MAXBUF);
  if (!buf) return 0;

  size = 0;
  while (1) {
    int n = read(0, buf+size, MAXBUF-(size+1));
    if (n <= 0) break;
    size += n;
  }

  if (size <= 0) return 0;

  buf[size] = 0;

  if (match_spam_strings(buf, size) ||
      match_spam_words(buf, size)) {
    char *spamfile = argv[1];
    int fd;

    fd = open(spamfile,O_CREAT|O_WRONLY|O_APPEND, 0666);
    if (fd == -1) {
      perror(spamfile);
      exit(1);
    }
    write(fd, buf, size);
    close(fd);
    return 0;
  }

  /* its OK, pass it on */
  write(1,buf,size);

  return 0;
}


