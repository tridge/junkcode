#include <stdio.h>

typedef int BOOL;
typedef char pstring[1024];
#define True 1
#define False 0
#define DEBUG(l,x)

/****************************************************************************
line strncpy but always null terminates. Make sure there is room!
****************************************************************************/
char *StrnCpy(char *dest,char *src,int n)
{
  char *d = dest;
  while (n-- && (*d++ = *src++)) ;
  *d = 0;
  return(dest);
}

/*******************************************************************
  convert a string to upper case
********************************************************************/
void strupper(char *s)
{
  while (*s)
    {
#ifdef KANJI
	if (is_shift_jis (*s)) {
	    s += 2;
	} else if (is_kana (*s)) {
	    s++;
	} else {
	    if (islower(*s))
		*s = toupper(*s);
	    s++;
	}
#else
      if (islower(*s))
	*s = toupper(*s);
      s++;
#endif
    }
}


static char *map_filename(char *s, /* This is null terminated */
			  char *pattern, /* This isn't. */
			  int len) /* This is the length of pattern. */
{
  static pstring matching_bit;  /* The bit of the string which matches */
                                /* a * in pattern if indeed there is a * */
  char *sp;                     /* Pointer into s. */
  char *pp;                     /* Pointer into p. */
  char *match_start;            /* Where the matching bit starts. */
  pstring pat;

  StrnCpy(pat, pattern, len);   /* Get pattern into a proper string! */
  strcpy(matching_bit,"");      /* Match but no star gets this. */
  pp = pat;                     /* Initialise the pointers. */
  sp = s;
  if ((len == 1) && (*pattern == '*')) {
    return NULL;                /* Impossible, too ambiguous for */
                                /* words! */
  }

  while ((*sp)                  /* Not the end of the string. */
         && (*pp)               /* Not the end of the pattern. */
         && (*sp == *pp)        /* The two match. */
         && (*pp != '*')) {     /* No wildcard. */
    sp++;                       /* Keep looking. */
    pp++;
  }
  if (!*sp && !*pp)             /* End of pattern. */
    return matching_bit;        /* Simple match.  Return empty string. */
  if (*pp == '*') {
    pp++;                       /* Always interrested in the chacter */
                                /* after the '*' */
    if (!*pp) {                 /* It is at the end of the pattern. */
      StrnCpy(matching_bit, s, sp-s);
      return matching_bit;
    } else {
      /* The next character in pattern must match a character further */
      /* along s than sp so look for that character. */
      match_start = sp;
      while ((*sp)              /* Not the end of s. */
             && (*sp != *pp))   /* Not the same  */
        sp++;                   /* Keep looking. */
      if (!*sp) {               /* Got to the end without a match. */
        return NULL;
      } else {                  /* Still hope for a match. */
        /* Now sp should point to a matching character. */
        StrnCpy(matching_bit, match_start, sp-match_start);
        /* Back to needing a stright match again. */
        while ((*sp)            /* Not the end of the string. */
               && (*pp)         /* Not the end of the pattern. */
               && (*sp == *pp)) { /* The two match. */
          sp++;                 /* Keep looking. */
          pp++;
        }
        if (!*sp && !*pp)       /* Both at end so it matched */
          return matching_bit;
        else
          return NULL;
      }
    }
  }
  return NULL;                  /* No match. */
}


static void do_fwd_mangled_map(char *s, char *MangledMap)
{
  /* MangledMap is a series of name pairs in () separated by spaces.
   * If s matches the first of the pair then the name given is the
   * second of the pair.  A * means any number of any character and if
   * present in the second of the pair as well as the first the
   * matching part of the first string takes the place of the * in the
   * second.
   *
   * I wanted this so that we could have RCS files which can be used
   * by UNIX and DOS programs.  My mapping string is (RCS rcs) which
   * converts the UNIX RCS file subdirectory to lowercase thus
   * preventing mangling.
   */
  char *start=MangledMap;       /* Use this to search for mappings. */
  char *end;                    /* Used to find the end of strings. */
  char *match_string;
  pstring new_string;           /* Make up the result here. */
  char *np;                     /* Points into new_string. */

  DEBUG(5,("Mangled Mapping '%s' map '%s'\n", s, MangledMap));
  while (*start) {
    while ((*start) && (*start != '('))
      start++;
    start++;                    /* Skip the ( */
    if (!*start)
      continue;                 /* Always check for the end. */
    end = start;                /* Search for the ' ' or a ')' */
    DEBUG(5,("Start of first in pair '%s'\n", start));
    while ((*end) && !((*end == ' ') || (*end == ')')))
      end++;
    if (!*end) {
      start = end;
      continue;                 /* Always check for the end. */
    }
    DEBUG(5,("End of first in pair '%s'\n", end));
    if ((match_string = map_filename(s, start, end-start))) {
      DEBUG(5,("Found a match\n"));
      /* Found a match. */
      start = end+1;            /* Point to start of what it is to become. */
      DEBUG(5,("Start of second in pair '%s'\n", start));
      end = start;
      np = new_string;
      while ((*end)             /* Not the end of string. */
             && (*end != ')')   /* Not the end of the pattern. */
             && (*end != '*'))  /* Not a wildcard. */
        *np++ = *end++;
      if (!*end) {
        start = end;
        continue;               /* Always check for the end. */
      }
      if (*end == '*') {
        strcpy(np, match_string);
        np += strlen(match_string);
        end++;                  /* Skip the '*' */
        while ((*end)             /* Not the end of string. */
               && (*end != ')')   /* Not the end of the pattern. */
               && (*end != '*'))  /* Not a wildcard. */
          *np++ = *end++;
      }
      if (!*end) {
        start = end;
        continue;               /* Always check for the end. */
      }
      *np++ = '\0';             /* NULL terminate it. */
      DEBUG(5,("End of second in pair '%s'\n", end));
      strcpy(s, new_string);    /* Substitute with the new name. */
      DEBUG(5,("s is now '%s'\n", s));
    }
    start = end;              /* Skip a bit which cannot be wanted */
    /* anymore. */
    start++;
  }
}


main(int argc,char *argv[])
{
  char name[100];

  strcpy(name,argv[2]);
  
  do_fwd_mangled_map(name,argv[1]);
  printf("name=%s\n",name);
}
