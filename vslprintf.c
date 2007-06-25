/* 
   Copyright (C) 1998 Andrew Tridgell
   Copyright (C) 1991, 92, 93, 94, 95, 96, 97 Free Software Foundation, Inc.

   This code is derived from code in the GNU C library, version
   2.0.6. It was modified by Andew Tridgell to provide a standalone
   vslprintf implementation portable across a wide range of
   machines. The modifications required some fairly heavy handed
   chainsawing of the code. If you think you can do a cleaner job then
   please do, and send me a patch (at tridge@samba.org)

   Note that some features of the glibc implementation were lost
   during the modifications. If you are looking for a full-featured
   glibc vsnprintf() routine then please go to the glibc sources. This
   code is intended for use by people writing secure applications for
   systems that may not have glibc installed.

   ------

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <locale.h>
#include <ctype.h>

#define CHAR_T		char
#define UCHAR_T	unsigned char
#define INT_T		int
#define L_(Str)	Str
#define ISDIGIT(Ch)	isdigit (Ch)

/* Codes returned by `parse_printf_format' for basic types.

   These values cover all the standard format specifications.
   Users can add new values after PA_LAST for their own types.  */

enum
{				/* C type: */
  PA_INT,			/* int */
  PA_CHAR,			/* int, cast to char */
  PA_WCHAR,			/* wide char */
  PA_STRING,			/* const char *, a '\0'-terminated string */
  PA_WSTRING,			/* const wchar_t *, wide character string */
  PA_POINTER,			/* void * */
  PA_FLOAT,			/* float */
  PA_DOUBLE,			/* double */
  PA_LAST
};

/* Flag bits that can be set in a type returned by `parse_printf_format'.  */
#define	PA_FLAG_MASK		0xff00
#define	PA_FLAG_LONG_LONG	(1 << 8)
#define	PA_FLAG_LONG_DOUBLE	PA_FLAG_LONG_LONG
#define	PA_FLAG_LONG		(1 << 9)
#define	PA_FLAG_SHORT		(1 << 10)
#define	PA_FLAG_PTR		(1 << 11)

struct printf_info
{
  int prec;			/* Precision.  */
  int width;			/* Width.  */
  wchar_t spec;			/* Format letter.  */
  unsigned int is_long_double:1;/* L flag.  */
  unsigned int is_short:1;	/* h flag.  */
  unsigned int is_long:1;	/* l flag.  */
  unsigned int alt:1;		/* # flag.  */
  unsigned int space:1;		/* Space flag.  */
  unsigned int left:1;		/* - flag.  */
  unsigned int showsign:1;	/* + flag.  */
  unsigned int group:1;		/* ' flag.  */
  unsigned int extra:1;		/* For special use.  */
  wchar_t pad;			/* Padding character.  */
};


/* we create a pseudo-file structure to make interfacing with the existing code
   easier */
typedef struct {
	char *s; /* pointer to the buffer */
	unsigned long offset; /* current offset within the buffer */
	unsigned long len; /* the size of the buffer */
} sFILE;

#define	outchar(Ch)							      \
  do									      \
    {									      \
      register const int outc = (Ch);					      \
      if (PUTC (outc, s) == EOF)					      \
	return -1;							      \
      else								      \
	++done;								      \
    }									      \
  while (0)

#define outstring(String, Len)						      \
  do									      \
    {									      \
      if ((size_t) PUT (s, (String), (Len)) != (size_t) (Len))		      \
	return -1;							      \
      done += (Len);							      \
    }									      \
  while (0)

/* For handling long_double and longlong we use the same flag.  */
#ifndef is_longlong
# define is_longlong is_long_double
#endif


/* Global variables.  */
static const char null[] = "(null)";


/* Helper function to provide temporary buffering for unbuffered streams.  */
static int buffered_vfprintf __P ((sFILE *stream, const CHAR_T *fmt, va_list));

/* Handle unknown format specifier.  */
static int printf_unknown __P ((FILE *, const struct printf_info *,
				const void *const *));

/* Group digits of number string.  */
static char *group_number __P ((CHAR_T *, CHAR_T *, const CHAR_T *, wchar_t));


struct printf_spec
  {
    /* Information parsed from the format spec.  */
    struct printf_info info;

    /* Pointers into the format string for the end of this format
       spec and the next (or to the end of the string if no more).  */
    const char *end_of_fmt, *next_fmt;

    /* Position of arguments for precision and width, or -1 if `info' has
       the constant value.  */
    int prec_arg, width_arg;

    int data_arg;		/* Position of data argument.  */
    int data_arg_type;		/* Type of first argument.  */
    /* Number of arguments consumed by this format specifier.  */
    size_t ndata_args;
  };


/* The various kinds off arguments that can be passed to printf.  */
union printf_arg
  {
    unsigned char pa_char;
    wchar_t pa_wchar;
    short int pa_short_int;
    int pa_int;
    long int pa_long_int;
    long long int pa_long_long_int;
    unsigned short int pa_u_short_int;
    unsigned int pa_u_int;
    unsigned long int pa_u_long_int;
    unsigned long long int pa_u_long_long_int;
    float pa_float;
    double pa_double;
    long double pa_long_double;
    const char *pa_string;
    const wchar_t *pa_wstring;
    void *pa_pointer;
  };


/* Read a simple integer from a string and update the string pointer.
   It is assumed that the first character is a digit.  */
static inline unsigned int
read_int (const UCHAR_T * *pstr)
{
  unsigned int retval = **pstr - L_('0');

  while (ISDIGIT (*++(*pstr)))
    {
      retval *= 10;
      retval += **pstr - L_('0');
    }

  return retval;
}



/* Find the next spec in FORMAT, or the end of the string.  Returns
   a pointer into FORMAT, to a '%' or a '\0'.  */
static inline const char *
find_spec (const char *format)
{
  while (*format != '\0' && *format != '%')
    {
	++format;
    }
  return format;
}

#ifndef UCHAR_MAX
#define UCHAR_MAX 255
#endif

/* Type of a printf specifier-handler function.
   STREAM is the FILE on which to write output.
   INFO gives information about the format specification.
   ARGS is a vector of pointers to the argument data;
   the number of pointers will be the number returned
   by the associated arginfo function for the same INFO.

   The function should return the number of characters written,
   or -1 for errors.  */

typedef int printf_function __P ((FILE *__stream,
				  __const struct printf_info *__info,
				  __const void *__const *__args));

/* Type of a printf specifier-arginfo function.
   INFO gives information about the format specification.
   N, ARGTYPES, and return value are as for printf_parse_format.  */

typedef int printf_arginfo_function __P ((__const struct printf_info *__info,
					  size_t __n,
					  int *__argtypes));

static printf_function *__printf_function_table[UCHAR_MAX + 1];
static printf_arginfo_function *__printf_arginfo_table[UCHAR_MAX + 1];


/* FORMAT must point to a '%' at the beginning of a spec.  Fills in *SPEC
   with the parsed details.  POSN is the number of arguments already
   consumed.  At most MAXTYPES - POSN types are filled in TYPES.  Return
   the number of args consumed by this spec; *MAX_REF_ARG is updated so it
   remains the highest argument index used.  */
static inline size_t
parse_one_spec (const UCHAR_T *format, size_t posn, struct printf_spec *spec,
		size_t *max_ref_arg)
{
  unsigned int n;
  size_t nargs = 0;

  /* Skip the '%'.  */
  ++format;

  /* Clear information structure.  */
  spec->data_arg = -1;
  spec->info.alt = 0;
  spec->info.space = 0;
  spec->info.left = 0;
  spec->info.showsign = 0;
  spec->info.group = 0;
  spec->info.pad = ' ';

  /* Test for positional argument.  */
  if (ISDIGIT (*format))
    {
      const UCHAR_T *begin = format;

      n = read_int (&format);

      if (n > 0 && *format == L_('$'))
	/* Is positional parameter.  */
	{
	  ++format;		/* Skip the '$'.  */
	  spec->data_arg = n - 1;
	  *max_ref_arg = MAX (*max_ref_arg, n);
	}
      else
	/* Oops; that was actually the width and/or 0 padding flag.
	   Step back and read it again.  */
	format = begin;
    }

  /* Check for spec modifiers.  */
  while (*format == L_(' ') || *format == L_('+') || *format == L_('-') ||
	 *format == L_('#') || *format == L_('0') || *format == L_('\''))
    switch (*format++)
      {
      case L_(' '):
	/* Output a space in place of a sign, when there is no sign.  */
	spec->info.space = 1;
	break;
      case L_('+'):
	/* Always output + or - for numbers.  */
	spec->info.showsign = 1;
	break;
      case L_('-'):
	/* Left-justify things.  */
	spec->info.left = 1;
	break;
      case L_('#'):
	/* Use the "alternate form":
	   Hex has 0x or 0X, FP always has a decimal point.  */
	spec->info.alt = 1;
	break;
      case L_('0'):
	/* Pad with 0s.  */
	spec->info.pad = '0';
	break;
      case L_('\''):
	/* Show grouping in numbers if the locale information
	   indicates any.  */
	spec->info.group = 1;
	break;
      }
  if (spec->info.left)
    spec->info.pad = ' ';

  /* Get the field width.  */
  spec->width_arg = -1;
  spec->info.width = 0;
  if (*format == L_('*'))
    {
      /* The field width is given in an argument.
	 A negative field width indicates left justification.  */
      const UCHAR_T *begin = ++format;

      if (ISDIGIT (*format))
	{
	  /* The width argument might be found in a positional parameter.  */
	  n = read_int (&format);

	  if (n > 0 && *format == L_('$'))
	    {
	      spec->width_arg = n - 1;
	      *max_ref_arg = MAX (*max_ref_arg, n);
	      ++format;		/* Skip '$'.  */
	    }
	}

      if (spec->width_arg < 0)
	{
	  /* Not in a positional parameter.  Consume one argument.  */
	  spec->width_arg = posn++;
	  ++nargs;
	  format = begin;	/* Step back and reread.  */
	}
    }
  else if (ISDIGIT (*format))
    /* Constant width specification.  */
    spec->info.width = read_int (&format);

  /* Get the precision.  */
  spec->prec_arg = -1;
  /* -1 means none given; 0 means explicit 0.  */
  spec->info.prec = -1;
  if (*format == L_('.'))
    {
      ++format;
      if (*format == L_('*'))
	{
	  /* The precision is given in an argument.  */
	  const UCHAR_T *begin = ++format;

	  if (ISDIGIT (*format))
	    {
	      n = read_int (&format);

	      if (n > 0 && *format == L_('$'))
		{
		  spec->prec_arg = n - 1;
		  *max_ref_arg = MAX (*max_ref_arg, n);
		  ++format;
		}
	    }

	  if (spec->prec_arg < 0)
	    {
	      /* Not in a positional parameter.  */
	      spec->prec_arg = posn++;
	      ++nargs;
	      format = begin;
	    }
	}
      else if (ISDIGIT (*format))
	spec->info.prec = read_int (&format);
      else
	/* "%.?" is treated like "%.0?".  */
	spec->info.prec = 0;
    }

  /* Check for type modifiers.  */
#define is_longlong is_long_double
  spec->info.is_long_double = 0;
  spec->info.is_short = 0;
  spec->info.is_long = 0;

  if (*format == L_('h') || *format == L_('l') || *format == L_('L') ||
      *format == L_('Z') || *format == L_('q'))
    switch (*format++)
      {
      case L_('h'):
	/* int's are short int's.  */
	spec->info.is_short = 1;
	break;
      case L_('l'):
	/* int's are long int's.  */
	spec->info.is_long = 1;
	if (*format != L_('l'))
	  break;
	++format;
	/* FALLTHROUGH */
      case L_('L'):
	/* double's are long double's, and int's are long long int's.  */
      case L_('q'):
	/* 4.4 uses this for long long.  */
	spec->info.is_long_double = 1;
	break;
      case L_('Z'):
	/* int's are size_t's.  */
	assert (sizeof(size_t) <= sizeof(unsigned long long int));
	spec->info.is_longlong = sizeof(size_t) > sizeof(unsigned long int);
	spec->info.is_long = sizeof(size_t) > sizeof(unsigned int);
	break;
      }

  /* Get the format specification.  */
  spec->info.spec = (wchar_t) *format++;
  if (__printf_function_table != NULL
      && spec->info.spec <= UCHAR_MAX
      && __printf_arginfo_table[spec->info.spec] != NULL)
    /* We don't try to get the types for all arguments if the format
       uses more than one.  The normal case is covered though.  */
    spec->ndata_args = (*__printf_arginfo_table[spec->info.spec])
      (&spec->info, 1, &spec->data_arg_type);
  else
    {
      /* Find the data argument types of a built-in spec.  */
      spec->ndata_args = 1;

      switch (spec->info.spec)
	{
	case L'i':
	case L'd':
	case L'u':
	case L'o':
	case L'X':
	case L'x':
	  if (spec->info.is_longlong)
	    spec->data_arg_type = PA_INT|PA_FLAG_LONG_LONG;
	  else if (spec->info.is_long)
	    spec->data_arg_type = PA_INT|PA_FLAG_LONG;
	  else if (spec->info.is_short)
	    spec->data_arg_type = PA_INT|PA_FLAG_SHORT;
	  else
	    spec->data_arg_type = PA_INT;
	  break;
	case L'e':
	case L'E':
	case L'f':
	case L'g':
	case L'G':
	  if (spec->info.is_long_double)
	    spec->data_arg_type = PA_DOUBLE|PA_FLAG_LONG_DOUBLE;
	  else
	    spec->data_arg_type = PA_DOUBLE;
	  break;
	case L'c':
	  spec->data_arg_type = PA_CHAR;
	  break;
	case L'C':
	  spec->data_arg_type = PA_WCHAR;
	  break;
	case L's':
	  spec->data_arg_type = PA_STRING;
	  break;
	case L'S':
	  spec->data_arg_type = PA_WSTRING;
	  break;
	case L'p':
	  spec->data_arg_type = PA_POINTER;
	  break;
	case L'n':
	  spec->data_arg_type = PA_INT|PA_FLAG_PTR;
	  break;

	case L'm':
	default:
	  /* An unknown spec will consume no args.  */
	  spec->ndata_args = 0;
	  break;
	}
    }

  if (spec->data_arg == -1 && spec->ndata_args > 0)
    {
      /* There are args consumed, but no positional spec.  Use the
	 next sequential arg position.  */
      spec->data_arg = posn;
      nargs += spec->ndata_args;
    }

  if (spec->info.spec == L'\0')
    /* Format ended before this spec was complete.  */
    spec->end_of_fmt = spec->next_fmt = format - 1;
  else
    {
      /* Find the next format spec.  */
      spec->end_of_fmt = format;
      spec->next_fmt = find_spec (format);
    }

  return nargs;
}


/* The function itself.  */
static int
_vslprintf (sFILE *s, const CHAR_T *format, va_list ap)
{
  /* The character used as thousands separator.  */
  wchar_t thousands_sep;

  /* The string describing the size of groups of digits.  */
  const char *grouping;

  /* Place to accumulate the result.  */
  int done;

  /* Current character in format string.  */
  const UCHAR_T *f;

  /* End of leading constant string.  */
  const UCHAR_T *lead_str_end;

  /* Points to next format specifier.  */
  const UCHAR_T *end_of_spec;

  /* Buffer intermediate results.  */
  char work_buffer[1000];
#define workend (&work_buffer[sizeof (work_buffer) - 1])

  /* We have to save the original argument pointer.  */
  va_list ap_save;

  /* Count number of specifiers we already processed.  */
  int nspecs_done;

  /* This table maps a character into a number representing a
     class.  In each step there is a destination label for each
     class.  */
  static const int jump_table[] =
  {
    /* ' ' */  1,            0,            0, /* '#' */  4,
	       0, /* '%' */ 14,            0, /* '\''*/  6,
	       0,            0, /* '*' */  7, /* '+' */  2,
	       0, /* '-' */  3, /* '.' */  9,            0,
    /* '0' */  5, /* '1' */  8, /* '2' */  8, /* '3' */  8,
    /* '4' */  8, /* '5' */  8, /* '6' */  8, /* '7' */  8,
    /* '8' */  8, /* '9' */  8,            0,            0,
	       0,            0,            0,            0,
	       0,            0,            0, /* 'C' */ 25,
	       0, /* 'E' */ 19,            0, /* 'G' */ 19,
	       0,            0,            0,            0,
    /* 'L' */ 12,            0,            0,            0,
	       0,            0,            0, /* 'S' */ 21,
	       0,            0,            0,            0,
    /* 'X' */ 18,            0, /* 'Z' */ 13,            0,
	       0,            0,            0,            0,
	       0,            0,            0, /* 'c' */ 20,
    /* 'd' */ 15, /* 'e' */ 19, /* 'f' */ 19, /* 'g' */ 19,
    /* 'h' */ 10, /* 'i' */ 15,            0,            0,
    /* 'l' */ 11, /* 'm' */ 24, /* 'n' */ 23, /* 'o' */ 17,
    /* 'p' */ 22, /* 'q' */ 12,            0, /* 's' */ 21,
	       0, /* 'u' */ 16,            0,            0,
    /* 'x' */ 18
  };

#define NOT_IN_JUMP_RANGE(Ch) ((Ch) < ' ' || (Ch) > 'x')
#define CHAR_CLASS(Ch) (jump_table[(int) (Ch) - ' '])
#define JUMP(ChExpr, table)						      \
      do								      \
	{								      \
	  const void *ptr;						      \
	  spec = (ChExpr);						      \
	  ptr = NOT_IN_JUMP_RANGE (spec) ? REF (form_unknown)		      \
	    : table[CHAR_CLASS (spec)];					      \
	  goto *ptr;							      \
	}								      \
      while (0)

#define STEP0_3_TABLE							      \
    /* Step 0: at the beginning.  */					      \
    static const void *step0_jumps[26] =				      \
    {									      \
      REF (form_unknown),						      \
      REF (flag_space),		/* for ' ' */				      \
      REF (flag_plus),		/* for '+' */				      \
      REF (flag_minus),		/* for '-' */				      \
      REF (flag_hash),		/* for '<hash>' */			      \
      REF (flag_zero),		/* for '0' */				      \
      REF (flag_quote),		/* for '\'' */				      \
      REF (width_asterics),	/* for '*' */				      \
      REF (width),		/* for '1'...'9' */			      \
      REF (precision),		/* for '.' */				      \
      REF (mod_half),		/* for 'h' */				      \
      REF (mod_long),		/* for 'l' */				      \
      REF (mod_longlong),	/* for 'L', 'q' */			      \
      REF (mod_size_t),		/* for 'Z' */				      \
      REF (form_percent),	/* for '%' */				      \
      REF (form_integer),	/* for 'd', 'i' */			      \
      REF (form_unsigned),	/* for 'u' */				      \
      REF (form_octal),		/* for 'o' */				      \
      REF (form_hexa),		/* for 'X', 'x' */			      \
      REF (form_float),		/* for 'E', 'e', 'f', 'G', 'g' */	      \
      REF (form_character),	/* for 'c' */				      \
      REF (form_string),	/* for 's', 'S' */			      \
      REF (form_pointer),	/* for 'p' */				      \
      REF (form_number),	/* for 'n' */				      \
      REF (form_strerror),	/* for 'm' */				      \
      REF (form_wcharacter)	/* for 'C' */				      \
    };									      \
    /* Step 1: after processing width.  */				      \
    static const void *step1_jumps[26] =				      \
    {									      \
      REF (form_unknown),						      \
      REF (form_unknown),	/* for ' ' */				      \
      REF (form_unknown),	/* for '+' */				      \
      REF (form_unknown),	/* for '-' */				      \
      REF (form_unknown),	/* for '<hash>' */			      \
      REF (form_unknown),	/* for '0' */				      \
      REF (form_unknown),	/* for '\'' */				      \
      REF (form_unknown),	/* for '*' */				      \
      REF (form_unknown),	/* for '1'...'9' */			      \
      REF (precision),		/* for '.' */				      \
      REF (mod_half),		/* for 'h' */				      \
      REF (mod_long),		/* for 'l' */				      \
      REF (mod_longlong),	/* for 'L', 'q' */			      \
      REF (mod_size_t),		/* for 'Z' */				      \
      REF (form_percent),	/* for '%' */				      \
      REF (form_integer),	/* for 'd', 'i' */			      \
      REF (form_unsigned),	/* for 'u' */				      \
      REF (form_octal),		/* for 'o' */				      \
      REF (form_hexa),		/* for 'X', 'x' */			      \
      REF (form_float),		/* for 'E', 'e', 'f', 'G', 'g' */	      \
      REF (form_character),	/* for 'c' */				      \
      REF (form_string),	/* for 's', 'S' */			      \
      REF (form_pointer),	/* for 'p' */				      \
      REF (form_number),	/* for 'n' */				      \
      REF (form_strerror),	/* for 'm' */				      \
      REF (form_wcharacter)	/* for 'C' */				      \
    };									      \
    /* Step 2: after processing precision.  */				      \
    static const void *step2_jumps[26] =				      \
    {									      \
      REF (form_unknown),						      \
      REF (form_unknown),	/* for ' ' */				      \
      REF (form_unknown),	/* for '+' */				      \
      REF (form_unknown),	/* for '-' */				      \
      REF (form_unknown),	/* for '<hash>' */			      \
      REF (form_unknown),	/* for '0' */				      \
      REF (form_unknown),	/* for '\'' */				      \
      REF (form_unknown),	/* for '*' */				      \
      REF (form_unknown),	/* for '1'...'9' */			      \
      REF (form_unknown),	/* for '.' */				      \
      REF (mod_half),		/* for 'h' */				      \
      REF (mod_long),		/* for 'l' */				      \
      REF (mod_longlong),	/* for 'L', 'q' */			      \
      REF (mod_size_t),		/* for 'Z' */				      \
      REF (form_percent),	/* for '%' */				      \
      REF (form_integer),	/* for 'd', 'i' */			      \
      REF (form_unsigned),	/* for 'u' */				      \
      REF (form_octal),		/* for 'o' */				      \
      REF (form_hexa),		/* for 'X', 'x' */			      \
      REF (form_float),		/* for 'E', 'e', 'f', 'G', 'g' */	      \
      REF (form_character),	/* for 'c' */				      \
      REF (form_string),	/* for 's', 'S' */			      \
      REF (form_pointer),	/* for 'p' */				      \
      REF (form_number),	/* for 'n' */				      \
      REF (form_strerror),	/* for 'm' */				      \
      REF (form_wcharacter)	/* for 'C' */				      \
    };									      \
    /* Step 3: after processing first 'l' modifier.  */			      \
    static const void *step3_jumps[26] =				      \
    {									      \
      REF (form_unknown),						      \
      REF (form_unknown),	/* for ' ' */				      \
      REF (form_unknown),	/* for '+' */				      \
      REF (form_unknown),	/* for '-' */				      \
      REF (form_unknown),	/* for '<hash>' */			      \
      REF (form_unknown),	/* for '0' */				      \
      REF (form_unknown),	/* for '\'' */				      \
      REF (form_unknown),	/* for '*' */				      \
      REF (form_unknown),	/* for '1'...'9' */			      \
      REF (form_unknown),	/* for '.' */				      \
      REF (form_unknown),	/* for 'h' */				      \
      REF (mod_longlong),	/* for 'l' */				      \
      REF (form_unknown),	/* for 'L', 'q' */			      \
      REF (form_unknown),	/* for 'Z' */				      \
      REF (form_percent),	/* for '%' */				      \
      REF (form_integer),	/* for 'd', 'i' */			      \
      REF (form_unsigned),	/* for 'u' */				      \
      REF (form_octal),		/* for 'o' */				      \
      REF (form_hexa),		/* for 'X', 'x' */			      \
      REF (form_float),		/* for 'E', 'e', 'f', 'G', 'g' */	      \
      REF (form_character),	/* for 'c' */				      \
      REF (form_string),	/* for 's', 'S' */			      \
      REF (form_pointer),	/* for 'p' */				      \
      REF (form_number),	/* for 'n' */				      \
      REF (form_strerror),	/* for 'm' */				      \
      REF (form_wcharacter)	/* for 'C' */				      \
    }

#define STEP4_TABLE							      \
    /* Step 4: processing format specifier.  */				      \
    static const void *step4_jumps[26] =				      \
    {									      \
      REF (form_unknown),						      \
      REF (form_unknown),	/* for ' ' */				      \
      REF (form_unknown),	/* for '+' */				      \
      REF (form_unknown),	/* for '-' */				      \
      REF (form_unknown),	/* for '<hash>' */			      \
      REF (form_unknown),	/* for '0' */				      \
      REF (form_unknown),	/* for '\'' */				      \
      REF (form_unknown),	/* for '*' */				      \
      REF (form_unknown),	/* for '1'...'9' */			      \
      REF (form_unknown),	/* for '.' */				      \
      REF (form_unknown),	/* for 'h' */				      \
      REF (form_unknown),	/* for 'l' */				      \
      REF (form_unknown),	/* for 'L', 'q' */			      \
      REF (form_unknown),	/* for 'Z' */				      \
      REF (form_percent),	/* for '%' */				      \
      REF (form_integer),	/* for 'd', 'i' */			      \
      REF (form_unsigned),	/* for 'u' */				      \
      REF (form_octal),		/* for 'o' */				      \
      REF (form_hexa),		/* for 'X', 'x' */			      \
      REF (form_float),		/* for 'E', 'e', 'f', 'G', 'g' */	      \
      REF (form_character),	/* for 'c' */				      \
      REF (form_string),	/* for 's', 'S' */			      \
      REF (form_pointer),	/* for 'p' */				      \
      REF (form_number),	/* for 'n' */				      \
      REF (form_strerror),	/* for 'm' */				      \
      REF (form_wcharacter)	/* for 'C' */				      \
    }


#define process_arg(fspec)						      \
      /* Start real work.  We know about all flags and modifiers and	      \
	 now process the wanted format specifier.  */			      \
    LABEL (form_percent):						      \
      /* Write a literal "%".  */					      \
      outchar ('%');							      \
      break;								      \
									      \
    LABEL (form_integer):						      \
      /* Signed decimal integer.  */					      \
      base = 10;							      \
									      \
      if (is_longlong)							      \
	{								      \
	  long long int signed_number;					      \
									      \
	  if (fspec == NULL)						      \
	    signed_number = va_arg (ap, long long int);			      \
	  else								      \
	    signed_number = args_value[fspec->data_arg].pa_long_long_int;     \
									      \
	  is_negative = signed_number < 0;				      \
	  number.longlong = is_negative ? (- signed_number) : signed_number;  \
									      \
	  goto LABEL (longlong_number);					      \
	}								      \
      else								      \
	{								      \
	  long int signed_number;					      \
									      \
	  if (fspec == NULL)						      \
	    if (is_long)						      \
	      signed_number = va_arg (ap, long int);			      \
	    else	/* `short int' will be promoted to `int'.  */	      \
	      signed_number = va_arg (ap, int);				      \
	  else								      \
	    if (is_long)						      \
	      signed_number = args_value[fspec->data_arg].pa_long_int;	      \
	    else							      \
	      signed_number = args_value[fspec->data_arg].pa_int;	      \
									      \
	  is_negative = signed_number < 0;				      \
	  number.word = is_negative ? (- signed_number) : signed_number;      \
									      \
	  goto LABEL (number);						      \
	}								      \
      /* NOTREACHED */							      \
									      \
    LABEL (form_unsigned):						      \
      /* Unsigned decimal integer.  */					      \
      base = 10;							      \
      goto LABEL (unsigned_number);					      \
      /* NOTREACHED */							      \
									      \
    LABEL (form_octal):							      \
      /* Unsigned octal integer.  */					      \
      base = 8;								      \
      goto LABEL (unsigned_number);					      \
      /* NOTREACHED */							      \
									      \
    LABEL (form_hexa):							      \
      /* Unsigned hexadecimal integer.  */				      \
      base = 16;							      \
									      \
    LABEL (unsigned_number):	  /* Unsigned number of base BASE.  */	      \
									      \
      /* ANSI specifies the `+' and ` ' flags only for signed		      \
	 conversions.  */						      \
      is_negative = 0;							      \
      showsign = 0;							      \
      space = 0;							      \
									      \
      if (is_longlong)							      \
	{								      \
	  if (fspec == NULL)						      \
	    number.longlong = va_arg (ap, unsigned long long int);	      \
	  else								      \
	    number.longlong = args_value[fspec->data_arg].pa_u_long_long_int; \
									      \
	LABEL (longlong_number):					      \
	  if (prec < 0)							      \
	    /* Supply a default precision if none was given.  */	      \
	    prec = 1;							      \
	  else								      \
	    /* We have to take care for the '0' flag.  If a precision	      \
	       is given it must be ignored.  */				      \
	    pad = ' ';							      \
									      \
	  /* If the precision is 0 and the number is 0 nothing has to	      \
	     be written for the number.  */				      \
	  if (prec == 0 && number.longlong == 0)			      \
	    string = workend;						      \
	  else								      \
	    {								      \
	      /* Put the number in WORK.  */				      \
	      string = _itoa (number.longlong, workend + 1, base,	      \
			      spec == 'X');				      \
	      string -= 1;						      \
	      if (group && grouping)					      \
		string = group_number (string, workend, grouping,	      \
				       thousands_sep);			      \
	    }								      \
	  /* Simply further test for num != 0.  */			      \
	  number.word = number.longlong != 0;				      \
	}								      \
      else								      \
	{								      \
	  if (fspec == NULL)						      \
	    if (is_long)						      \
	      number.word = va_arg (ap, unsigned long int);		      \
	    else if (!is_short)						      \
	      number.word = va_arg (ap, unsigned int);			      \
	    else							      \
	      number.word = (unsigned short int) va_arg (ap, unsigned int);   \
	  else								      \
	    if (is_long)						      \
	      number.word = args_value[fspec->data_arg].pa_u_long_int;	      \
	    else if (!is_short)						      \
	      number.word = args_value[fspec->data_arg].pa_u_int;	      \
	    else							      \
	      number.word = (unsigned short int)			      \
		args_value[fspec->data_arg].pa_u_short_int;		      \
									      \
	LABEL (number):							      \
	  if (prec < 0)							      \
	    /* Supply a default precision if none was given.  */	      \
	    prec = 1;							      \
	  else								      \
	    /* We have to take care for the '0' flag.  If a precision	      \
	       is given it must be ignored.  */				      \
	    pad = ' ';							      \
									      \
	  /* If the precision is 0 and the number is 0 nothing has to	      \
	     be written for the number.  */				      \
	  if (prec == 0 && number.word == 0)				      \
	    string = workend;						      \
	  else								      \
	    {								      \
	      /* Put the number in WORK.  */				      \
	      string = _itoa_word (number.word, workend + 1, base,	      \
				   spec == 'X');			      \
	      string -= 1;						      \
	      if (group && grouping)					      \
		string = group_number (string, workend, grouping,	      \
				       thousands_sep);			      \
	    }								      \
	}								      \
									      \
      prec -= workend - string;						      \
									      \
      if (prec > 0)							      \
	/* Add zeros to the precision.  */				      \
	while (prec-- > 0)						      \
	  *string-- = '0';						      \
      else if (number.word != 0 && alt && base == 8)			      \
	/* Add octal marker.  */					      \
	*string-- = '0';						      \
									      \
      if (!left)							      \
	{								      \
	  width -= workend - string;					      \
									      \
	  if (number.word != 0 && alt && base == 16)			      \
	    /* Account for 0X hex marker.  */				      \
	    width -= 2;							      \
									      \
	  if (is_negative || showsign || space)				      \
	    --width;							      \
									      \
	  if (pad == '0')						      \
	    {								      \
	      while (width-- > 0)					      \
		*string-- = '0';					      \
									      \
	      if (number.word != 0 && alt && base == 16)		      \
		{							      \
		  *string-- = spec;					      \
		  *string-- = '0';					      \
		}							      \
									      \
	      if (is_negative)						      \
		*string-- = '-';					      \
	      else if (showsign)					      \
		*string-- = '+';					      \
	      else if (space)						      \
		*string-- = ' ';					      \
	    }								      \
	  else								      \
	    {								      \
	      if (number.word != 0 && alt && base == 16)		      \
		{							      \
		  *string-- = spec;					      \
		  *string-- = '0';					      \
		}							      \
									      \
	      if (is_negative)						      \
		*string-- = '-';					      \
	      else if (showsign)					      \
		*string-- = '+';					      \
	      else if (space)						      \
		*string-- = ' ';					      \
									      \
	      while (width-- > 0)					      \
		*string-- = ' ';					      \
	    }								      \
									      \
	  outstring (string + 1, workend - string);			      \
									      \
	  break;							      \
	}								      \
      else								      \
	{								      \
	  if (number.word != 0 && alt && base == 16)			      \
	    {								      \
	      *string-- = spec;						      \
	      *string-- = '0';						      \
	    }								      \
									      \
	  if (is_negative)						      \
	    *string-- = '-';						      \
	  else if (showsign)						      \
	    *string-- = '+';						      \
	  else if (space)						      \
	    *string-- = ' ';						      \
									      \
	  width -= workend - string;					      \
	  outstring (string + 1, workend - string);			      \
									      \
	  PAD (' ');							      \
	  break;							      \
	}								      \
									      \
    LABEL (form_float):							      \
      {									      \
	/* Floating-point number.  This is handled by printf_fp.c.  */	      \
	extern int __printf_fp __P ((FILE *, const struct printf_info *,      \
				     const void **const));		      \
	const void *ptr;						      \
	int function_done;						      \
									      \
	if (fspec == NULL)						      \
	  {								      \
	    struct printf_info info = { prec: prec,			      \
					width: width,			      \
					spec: spec,			      \
					is_long_double: is_long_double,	      \
					is_short: is_short,		      \
					is_long: is_long,		      \
					alt: alt,			      \
					space: space,			      \
					left: left,			      \
					showsign: showsign,		      \
					group: group,			      \
					pad: pad,			      \
					extra: 0 };			      \
									      \
	    if (is_long_double)						      \
	      the_arg.pa_long_double = va_arg (ap, long double);	      \
	    else							      \
	      the_arg.pa_double = va_arg (ap, double);			      \
	    ptr = (const void *) &the_arg;				      \
									      \
	    function_done = __printf_fp (s, &info, &ptr);		      \
	  }								      \
	else								      \
	  {								      \
	    ptr = (const void *) &args_value[fspec->data_arg];		      \
									      \
	    function_done = __printf_fp (s, &fspec->info, &ptr);	      \
	  }								      \
									      \
	if (function_done < 0)						      \
	  /* Error in print handler.  */				      \
	  return -1;							      \
									      \
	done += function_done;						      \
      }									      \
      break;								      \
									      \
    LABEL (form_character):						      \
      /* Character.  */							      \
      if (is_long)							      \
	goto LABEL (form_wcharacter);					      \
      --width;	/* Account for the character itself.  */		      \
      if (!left)							      \
	PAD (' ');							      \
      if (fspec == NULL)						      \
	outchar ((unsigned char) va_arg (ap, int)); /* Promoted.  */	      \
      else								      \
	outchar ((unsigned char) args_value[fspec->data_arg].pa_char);	      \
      if (left)								      \
	PAD (' ');							      \
      break;								      \
									      \
    LABEL (form_string):						      \
      {									      \
	size_t len;							      \
									      \
	/* The string argument could in fact be `char *' or `wchar_t *'.      \
	   But this should not make a difference here.  */		      \
	if (fspec == NULL)						      \
	  string = (char *) va_arg (ap, const char *);			      \
	else								      \
	  string = (char *) args_value[fspec->data_arg].pa_string;	      \
									      \
	/* Entry point for printing other strings.  */			      \
      LABEL (print_string):						      \
									      \
	if (string == NULL)						      \
	  {								      \
	    /* Write "(null)" if there's space.  */			      \
	    if (prec == -1 || prec >= (int) sizeof (null) - 1)		      \
	      {								      \
		string = (char *) null;					      \
		len = sizeof (null) - 1;				      \
	      }								      \
	    else							      \
	      {								      \
		string = (char *) "";					      \
		len = 0;						      \
	      }								      \
	  }								      \
	else if (!is_long && spec != L_('S'))				      \
	  {								      \
	    if (prec != -1)						      \
	      /* Search for the end of the string, but don't search past      \
		 the length specified by the precision.  */		      \
	      len = strnlen (string, prec);				      \
	    else							      \
	      len = strlen (string);					      \
	  }								      \
									      \
	if ((width -= len) < 0)						      \
	  {								      \
	    outstring (string, len);					      \
	    break;							      \
	  }								      \
									      \
	if (!left)							      \
	  PAD (' ');							      \
	outstring (string, len);					      \
	if (left)							      \
	  PAD (' ');							      \
      }									      \
      break;								      \
									      \
    LABEL (form_pointer):						      \
      /* Generic pointer.  */						      \
      {									      \
	const void *ptr;						      \
	if (fspec == NULL)						      \
	  ptr = va_arg (ap, void *);					      \
	else								      \
	  ptr = args_value[fspec->data_arg].pa_pointer;			      \
	if (ptr != NULL)						      \
	  {								      \
	    /* If the pointer is not NULL, write it as a %#x spec.  */	      \
	    base = 16;							      \
	    number.word = (unsigned long int) ptr;			      \
	    is_negative = 0;						      \
	    alt = 1;							      \
	    group = 0;							      \
	    spec = 'x';							      \
	    goto LABEL (number);					      \
	  }								      \
	else								      \
	  {								      \
	    /* Write "(nil)" for a nil pointer.  */			      \
	    string = (char *) "(nil)";					      \
	    /* Make sure the full string "(nil)" is printed.  */	      \
	    if (prec < 5)						      \
	      prec = 5;							      \
	    is_long = 0;	/* This is no wide-char string.  */	      \
	    goto LABEL (print_string);					      \
	  }								      \
      }									      \
      /* NOTREACHED */							      \
									      \
    LABEL (form_number):						      \
      /* Answer the count of characters written.  */			      \
      if (fspec == NULL)						      \
	if (is_longlong)						      \
	  *(long long int *) va_arg (ap, void *) = done;		      \
	else if (is_long)						      \
	  *(long int *) va_arg (ap, void *) = done;			      \
	else if (!is_short)						      \
	  *(int *) va_arg (ap, void *) = done;				      \
	else								      \
	  *(short int *) va_arg (ap, void *) = done;			      \
      else								      \
	if (is_longlong)						      \
	  *(long long int *) args_value[fspec->data_arg].pa_pointer = done;   \
	else if (is_long)						      \
	  *(long int *) args_value[fspec->data_arg].pa_pointer = done;	      \
	else if (!is_short)						      \
	  *(int *) args_value[fspec->data_arg].pa_pointer = done;	      \
	else								      \
	  *(short int *) args_value[fspec->data_arg].pa_pointer = done;	      \
      break;


  /* Sanity check of arguments.  */
  ARGCHECK (s, format);

  if (UNBUFFERED_P (s))
    /* Use a helper function which will allocate a local temporary buffer
       for the stream and then call us again.  */
    return buffered_vfprintf (s, format, ap);

  /* Initialize local variables.  */
  done = 0;
  grouping = (const char *) -1;
#ifdef __va_copy
  /* This macro will be available soon in gcc's <stdarg.h>.  Me need it
     since on some systems `va_list' is not an integral type.  */
  __va_copy (ap_save, ap);
#else
  ap_save = ap;
#endif
  nspecs_done = 0;

  /* Find the first format specifier.  */
  f = lead_str_end = find_spec (format);

  /* Write the literal text before the first format.  */
  outstring ((const UCHAR_T *) format,
	     lead_str_end - (const UCHAR_T *) format);

  /* If we only have to print a simple string, return now.  */
  if (*f == L_('\0'))
    goto all_done;

  /* Process whole format string.  */
  do
    {
#define REF(Name) &&do_##Name
#define LABEL(Name) do_##Name
      STEP0_3_TABLE;
      STEP4_TABLE;

      union printf_arg *args_value;	/* This is not used here but ... */
      int is_negative;	/* Flag for negative number.  */
      union
      {
	unsigned long long int longlong;
	unsigned long int word;
      } number;
      int base;
      union printf_arg the_arg;
      char *string;	/* Pointer to argument string.  */
      int alt = 0;	/* Alternate format.  */
      int space = 0;	/* Use space prefix if no sign is needed.  */
      int left = 0;	/* Left-justify output.  */
      int showsign = 0;	/* Always begin with plus or minus sign.  */
      int group = 0;	/* Print numbers according grouping rules.  */
      int is_long_double = 0; /* Argument is long double/ long long int.  */
      int is_short = 0;	/* Argument is long int.  */
      int is_long = 0;	/* Argument is short int.  */
      int width = 0;	/* Width of output; 0 means none specified.  */
      int prec = -1;	/* Precision of output; -1 means none specified.  */
      char pad = ' ';	/* Padding character.  */
      CHAR_T spec;

      /* Get current character in format string.  */
      JUMP (*++f, step0_jumps);

      /* ' ' flag.  */
    LABEL (flag_space):
      space = 1;
      JUMP (*++f, step0_jumps);

      /* '+' flag.  */
    LABEL (flag_plus):
      showsign = 1;
      JUMP (*++f, step0_jumps);

      /* The '-' flag.  */
    LABEL (flag_minus):
      left = 1;
      pad = L_(' ');
      JUMP (*++f, step0_jumps);

      /* The '#' flag.  */
    LABEL (flag_hash):
      alt = 1;
      JUMP (*++f, step0_jumps);

      /* The '0' flag.  */
    LABEL (flag_zero):
      if (!left)
	pad = L_('0');
      JUMP (*++f, step0_jumps);

      /* The '\'' flag.  */
    LABEL (flag_quote):
      group = 1;

      /* XXX Completely wrong.  Use wctob.  */
      if (grouping == (const char *) -1)
	{
		grouping = "";
		thousands_sep = 0;
	}
      JUMP (*++f, step0_jumps);

      /* Get width from argument.  */
    LABEL (width_asterics):
      {
	const UCHAR_T *tmp;	/* Temporary value.  */

	tmp = ++f;
	if (ISDIGIT (*tmp) && read_int (&tmp) && *tmp == L_('$'))
	  /* The width comes from a positional parameter.  */
	  goto do_positional;

	width = va_arg (ap, int);

	/* Negative width means left justified.  */
	if (width < 0)
	  {
	    width = -width;
	    pad = L_(' ');
	    left = 1;
	  }
      }
      JUMP (*f, step1_jumps);

      /* Given width in format string.  */
    LABEL (width):
      width = read_int (&f);
      if (*f == L_('$'))
	/* Oh, oh.  The argument comes from a positional parameter.  */
	goto do_positional;
      JUMP (*f, step1_jumps);

    LABEL (precision):
      ++f;
      if (*f == L_('*'))
	{
	  const UCHAR_T *tmp;	/* Temporary value.  */

	  tmp = ++f;
	  if (ISDIGIT (*tmp) && read_int (&tmp) > 0 && *tmp == L_('$'))
	    /* The precision comes from a positional parameter.  */
	    goto do_positional;

	  prec = va_arg (ap, int);

	  /* If the precision is negative the precision is omitted.  */
	  if (prec < 0)
	    prec = -1;
	}
      else if (ISDIGIT (*f))
	prec = read_int (&f);
      else
	prec = 0;
      JUMP (*f, step2_jumps);

      /* Process 'h' modifier.  No other modifier is allowed to
	 follow.  */
    LABEL (mod_half):
      is_short = 1;
      JUMP (*++f, step4_jumps);

      /* Process 'l' modifier.  There might another 'l' follow.  */
    LABEL (mod_long):
      is_long = 1;
      JUMP (*++f, step3_jumps);

      /* Process 'L', 'q', or 'll' modifier.  No other modifier is
	 allowed to follow.  */
    LABEL (mod_longlong):
      is_long_double = 1;
      JUMP (*++f, step4_jumps);

    LABEL (mod_size_t):
      is_longlong = sizeof (size_t) > sizeof (unsigned long int);
      is_long = sizeof (size_t) > sizeof (unsigned int);
      JUMP (*++f, step4_jumps);


      /* Process current format.  */
      while (1)
	{
	  process_arg (((struct printf_spec *) NULL));

	LABEL (form_unknown):
	  if (spec == L_('\0'))
	    {
	      /* The format string ended before the specifier is complete.  */
	      done = -1;
	      goto all_done;
	    }

	  /* If we are in the fast loop force entering the complicated
	     one.  */
	  goto do_positional;
	}

      /* The format is correctly handled.  */
      ++nspecs_done;

      /* Look for next format specifier.  */
      f = find_spec ((end_of_spec = ++f));

      /* Write the following constant string.  */
      outstring (end_of_spec, f - end_of_spec);
    }
  while (*f != L_('\0'));

  /* Unlock stream and return.  */
  goto all_done;

  /* Here starts the more complex loop to handle positional parameters.  */
do_positional:
  {
    /* Array with information about the needed arguments.  This has to
       be dynamically extensible.  */
    size_t nspecs = 0;
    size_t nspecs_max = 32;	/* A more or less arbitrary start value.  */
    struct printf_spec *specs
      = alloca (nspecs_max * sizeof (struct printf_spec));

    /* The number of arguments the format string requests.  This will
       determine the size of the array needed to store the argument
       attributes.  */
    size_t nargs = 0;
    int *args_type;
    union printf_arg *args_value;

    /* Positional parameters refer to arguments directly.  This could
       also determine the maximum number of arguments.  Track the
       maximum number.  */
    size_t max_ref_arg = 0;

    /* Just a counter.  */
    size_t cnt;


    if (grouping == (const char *) -1)
      {
	      grouping = "";
	      thousands_sep = 0;
      }

    for (f = lead_str_end; *f != '\0'; f = specs[nspecs++].next_fmt)
      {
	if (nspecs >= nspecs_max)
	  {
	    /* Extend the array of format specifiers.  */
	    struct printf_spec *old = specs;

	    nspecs_max *= 2;
	    specs = alloca (nspecs_max * sizeof (struct printf_spec));

	    if (specs == &old[nspecs])
	      /* Stack grows up, OLD was the last thing allocated;
		 extend it.  */
	      nspecs_max += nspecs_max / 2;
	    else
	      {
		/* Copy the old array's elements to the new space.  */
		memcpy (specs, old, nspecs * sizeof (struct printf_spec));
		if (old == &specs[nspecs])
		  /* Stack grows down, OLD was just below the new
		     SPECS.  We can use that space when the new space
		     runs out.  */
		  nspecs_max += nspecs_max / 2;
	      }
	  }

	/* Parse the format specifier.  */
	nargs += parse_one_spec (f, nargs, &specs[nspecs], &max_ref_arg);
      }

    /* Determine the number of arguments the format string consumes.  */
    nargs = MAX (nargs, max_ref_arg);

    /* Allocate memory for the argument descriptions.  */
    args_type = alloca (nargs * sizeof (int));
    memset (args_type, 0, nargs * sizeof (int));
    args_value = alloca (nargs * sizeof (union printf_arg));

    /* XXX Could do sanity check here: If any element in ARGS_TYPE is
       still zero after this loop, format is invalid.  For now we
       simply use 0 as the value.  */

    /* Fill in the types of all the arguments.  */
    for (cnt = 0; cnt < nspecs; ++cnt)
      {
	/* If the width is determined by an argument this is an int.  */
	if (specs[cnt].width_arg != -1)
	  args_type[specs[cnt].width_arg] = PA_INT;

	/* If the precision is determined by an argument this is an int.  */
	if (specs[cnt].prec_arg != -1)
	  args_type[specs[cnt].prec_arg] = PA_INT;

	switch (specs[cnt].ndata_args)
	  {
	  case 0:		/* No arguments.  */
	    break;
	  case 1:		/* One argument; we already have the type.  */
	    args_type[specs[cnt].data_arg] = specs[cnt].data_arg_type;
	    break;
	  default:
	    /* We have more than one argument for this format spec.
	       We must call the arginfo function again to determine
	       all the types.  */
	    (void) (*__printf_arginfo_table[specs[cnt].info.spec])
	      (&specs[cnt].info,
	       specs[cnt].ndata_args, &args_type[specs[cnt].data_arg]);
	    break;
	  }
      }

    /* Now we know all the types and the order.  Fill in the argument
       values.  */
    for (cnt = 0; cnt < nargs; ++cnt)
      switch (args_type[cnt])
	{
#define T(tag, mem, type)						      \
	case tag:							      \
	  args_value[cnt].mem = va_arg (ap_save, type);			      \
	  break

	T (PA_CHAR, pa_char, int); /* Promoted.  */
	T (PA_WCHAR, pa_wchar, wint_t);
	T (PA_INT|PA_FLAG_SHORT, pa_short_int, int); /* Promoted.  */
	T (PA_INT, pa_int, int);
	T (PA_INT|PA_FLAG_LONG, pa_long_int, long int);
	T (PA_INT|PA_FLAG_LONG_LONG, pa_long_long_int, long long int);
	T (PA_FLOAT, pa_float, double);	/* Promoted.  */
	T (PA_DOUBLE, pa_double, double);
	T (PA_DOUBLE|PA_FLAG_LONG_DOUBLE, pa_long_double, long double);
	T (PA_STRING, pa_string, const char *);
	T (PA_WSTRING, pa_wstring, const wchar_t *);
	T (PA_POINTER, pa_pointer, void *);
#undef T
	default:
	  if ((args_type[cnt] & PA_FLAG_PTR) != 0)
	    args_value[cnt].pa_pointer = va_arg (ap_save, void *);
	  else
	    args_value[cnt].pa_long_double = 0.0;
	  break;
	}

    /* Now walk through all format specifiers and process them.  */
    for (; (size_t) nspecs_done < nspecs; ++nspecs_done)
      {
#undef REF
#define REF(Name) &&do2_##Name
#undef LABEL
#define LABEL(Name) do2_##Name
	STEP4_TABLE;

	int is_negative;
	union
	{
	  unsigned long long int longlong;
	  unsigned long int word;
	} number;
	int base;
	union printf_arg the_arg;
	char *string;	/* Pointer to argument string.  */

	/* Fill variables from values in struct.  */
	int alt = specs[nspecs_done].info.alt;
	int space = specs[nspecs_done].info.space;
	int left = specs[nspecs_done].info.left;
	int showsign = specs[nspecs_done].info.showsign;
	int group = specs[nspecs_done].info.group;
	int is_long_double = specs[nspecs_done].info.is_long_double;
	int is_short = specs[nspecs_done].info.is_short;
	int is_long = specs[nspecs_done].info.is_long;
	int width = specs[nspecs_done].info.width;
	int prec = specs[nspecs_done].info.prec;
	char pad = specs[nspecs_done].info.pad;
	CHAR_T spec = specs[nspecs_done].info.spec;

	/* Fill in last information.  */
	if (specs[nspecs_done].width_arg != -1)
	  {
	    /* Extract the field width from an argument.  */
	    specs[nspecs_done].info.width =
	      args_value[specs[nspecs_done].width_arg].pa_int;

	    if (specs[nspecs_done].info.width < 0)
	      /* If the width value is negative left justification is
		 selected and the value is taken as being positive.  */
	      {
		specs[nspecs_done].info.width *= -1;
		left = specs[nspecs_done].info.left = 1;
	      }
	    width = specs[nspecs_done].info.width;
	  }

	if (specs[nspecs_done].prec_arg != -1)
	  {
	    /* Extract the precision from an argument.  */
	    specs[nspecs_done].info.prec =
	      args_value[specs[nspecs_done].prec_arg].pa_int;

	    if (specs[nspecs_done].info.prec < 0)
	      /* If the precision is negative the precision is
		 omitted.  */
	      specs[nspecs_done].info.prec = -1;

	    prec = specs[nspecs_done].info.prec;
	  }

	/* Process format specifiers.  */
	while (1)
	  {
	    JUMP (spec, step4_jumps);

	    process_arg ((&specs[nspecs_done]));

	  LABEL (form_unknown):
	    {
	      int function_done;
	      printf_function *function;
	      unsigned int i;
	      const void **ptr;

	      function =
		(__printf_function_table == NULL ? NULL :
		 __printf_function_table[specs[nspecs_done].info.spec]);

	      if (function == NULL)
		function = &printf_unknown;

	      ptr = alloca (specs[nspecs_done].ndata_args
			    * sizeof (const void *));

	      /* Fill in an array of pointers to the argument values.  */
	      for (i = 0; i < specs[nspecs_done].ndata_args; ++i)
		ptr[i] = &args_value[specs[nspecs_done].data_arg + i];

	      /* Call the function.  */
	      function_done = (*function) (s, &specs[nspecs_done].info, ptr);

	      /* If an error occurred we don't have information about #
		 of chars.  */
	      if (function_done < 0)
		{
		  done = -1;
		  goto all_done;
		}

	      done += function_done;
	    }
	    break;
	  }

	/* Write the following constant string.  */
	outstring (specs[nspecs_done].end_of_fmt,
		   specs[nspecs_done].next_fmt
		   - specs[nspecs_done].end_of_fmt);
      }
  }

all_done:
  /* Unlock the stream.  */
  __libc_cleanup_region_end (1);

  return done;
}


/* Handle an unknown format specifier.  This prints out a canonicalized
   representation of the format spec itself.  */
static int
printf_unknown (sFILE *s, const struct printf_info *info,
		const void *const *args)

{
  int done = 0;
  char work_buffer[BUFSIZ];
  register char *w;

  outchar ('%');

  if (info->alt)
    outchar ('#');
  if (info->group)
    outchar ('\'');
  if (info->showsign)
    outchar ('+');
  else if (info->space)
    outchar (' ');
  if (info->left)
    outchar ('-');
  if (info->pad == '0')
    outchar ('0');

  if (info->width != 0)
    {
      w = _itoa_word (info->width, workend + 1, 10, 0);
      while (w <= workend)
	outchar (*w++);
    }

  if (info->prec != -1)
    {
      outchar ('.');
      w = _itoa_word (info->prec, workend + 1, 10, 0);
      while (w <= workend)
	outchar (*w++);
    }

  if (info->spec != '\0')
    outchar (info->spec);

  return done;
}


/* Group the digits according to the grouping rules of the current locale.
   The interpretation of GROUPING is as in `struct lconv' from <locale.h>.  */
static char *
group_number (CHAR_T *w, CHAR_T *rear_ptr, const CHAR_T *grouping,
	      wchar_t thousands_sep)
{
  int len;
  char *src, *s;

  /* We treat all negative values like CHAR_MAX.  */

  if (*grouping == CHAR_MAX || *grouping <= 0)
    /* No grouping should be done.  */
    return w;

  len = *grouping;

  /* Copy existing string so that nothing gets overwritten.  */
  src = (char *) alloca (rear_ptr - w);
  memcpy (src, w + 1, rear_ptr - w);
  s = &src[rear_ptr - w - 1];
  w = rear_ptr;

  /* Process all characters in the string.  */
  while (s >= src)
    {
      *w-- = *s--;

      if (--len == 0 && s >= src)
	{
	  /* A new group begins.  */
	  *w-- = thousands_sep;

	  len = *grouping++;
	  if (*grouping == '\0')
	    /* The previous grouping repeats ad infinitum.  */
	    --grouping;
	  else if (*grouping == CHAR_MAX || *grouping < 0)
	    {
	      /* No further grouping to be done.
		 Copy the rest of the number.  */
	      do
		*w-- = *s--;
	      while (s >= src);
	      break;
	    }
	}
    }
  return w;
}

#ifdef USE_IN_LIBIO
/* Helper "class" for `fprintf to unbuffered': creates a temporary buffer.  */
struct helper_file
  {
    struct _IO_FILE_plus _f;
    _IO_FILE *_put_stream;
#ifdef _IO_MTSAFE_IO
    _IO_lock_t lock;
#endif
  };

static int
_IO_helper_overflow (_IO_FILE *s, int c)
{
  _IO_FILE *target = ((struct helper_file*) s)->_put_stream;
  int used = s->_IO_write_ptr - s->_IO_write_base;
  if (used)
    {
      _IO_size_t written = _IO_sputn (target, s->_IO_write_base, used);
      s->_IO_write_ptr -= written;
    }
  return PUTC (c, s);
}

static const struct _IO_jump_t _IO_helper_jumps =
{
  JUMP_INIT_DUMMY,
  JUMP_INIT (finish, _IO_default_finish),
  JUMP_INIT (overflow, _IO_helper_overflow),
  JUMP_INIT (underflow, _IO_default_underflow),
  JUMP_INIT (uflow, _IO_default_uflow),
  JUMP_INIT (pbackfail, _IO_default_pbackfail),
  JUMP_INIT (xsputn, _IO_default_xsputn),
  JUMP_INIT (xsgetn, _IO_default_xsgetn),
  JUMP_INIT (seekoff, _IO_default_seekoff),
  JUMP_INIT (seekpos, _IO_default_seekpos),
  JUMP_INIT (setbuf, _IO_default_setbuf),
  JUMP_INIT (sync, _IO_default_sync),
  JUMP_INIT (doallocate, _IO_default_doallocate),
  JUMP_INIT (read, _IO_default_read),
  JUMP_INIT (write, _IO_default_write),
  JUMP_INIT (seek, _IO_default_seek),
  JUMP_INIT (close, _IO_default_close),
  JUMP_INIT (stat, _IO_default_stat)
};

static int
buffered_vfprintf (sFILE *s, const CHAR_T *format,
		   _IO_va_list args)
{
  char buf[_IO_BUFSIZ];
  struct helper_file helper;
  register _IO_FILE *hp = (_IO_FILE *) &helper;
  int result, to_flush;

  /* Initialize helper.  */
  helper._put_stream = s;
  hp->_IO_write_base = buf;
  hp->_IO_write_ptr = buf;
  hp->_IO_write_end = buf + sizeof buf;
  hp->_IO_file_flags = _IO_MAGIC|_IO_NO_READS;
#ifdef _IO_MTSAFE_IO
  hp->_lock = &helper.lock;
  __libc_lock_init (*hp->_lock);
#endif
  _IO_JUMPS (hp) = (struct _IO_jump_t *) &_IO_helper_jumps;

  /* Now print to helper instead.  */
  result = _IO_vfprintf (hp, format, args);

  /* Now flush anything from the helper to the S. */
  if ((to_flush = hp->_IO_write_ptr - hp->_IO_write_base) > 0)
    {
      if ((int) _IO_sputn (s, hp->_IO_write_base, to_flush) != to_flush)
	return -1;
    }

  return result;
}

#else /* !USE_IN_LIBIO */

static int
buffered_vfprintf (register FILE *s, const CHAR_T *format, va_list args)
{
  char buf[BUFSIZ];
  int result;

  s->__bufp = s->__buffer = buf;
  s->__bufsize = sizeof buf;
  s->__put_limit = s->__buffer + s->__bufsize;
  s->__get_limit = s->__buffer;

  /* Now use buffer to print.  */
  result = vfprintf (s, format, args);

  if (fflush (s) == EOF)
    result = -1;
  s->__buffer = s->__bufp = s->__get_limit = s->__put_limit = NULL;
  s->__bufsize = 0;

  return result;
}

/* Pads string with given number of a specified character.
   This code is taken from iopadn.c of the GNU I/O library.  */
#define PADSIZE 16
static const CHAR_T blanks[PADSIZE] =
{ L_(' '), L_(' '), L_(' '), L_(' '), L_(' '), L_(' '), L_(' '), L_(' '),
  L_(' '), L_(' '), L_(' '), L_(' '), L_(' '), L_(' '), L_(' '), L_(' ') };
static const CHAR_T zeroes[PADSIZE] =
{ L_('0'), L_('0'), L_('0'), L_('0'), L_('0'), L_('0'), L_('0'), L_('0'),
  L_('0'), L_('0'), L_('0'), L_('0'), L_('0'), L_('0'), L_('0'), L_('0') };

ssize_t
#ifndef COMPILE_WPRINTF
__printf_pad (FILE *s, char pad, size_t count)
#else
__wprintf_pad (FILE *s, wchar_t pad, size_t count)
#endif
{
  const CHAR_T *padptr;
  register size_t i;

  padptr = pad == L_(' ') ? blanks : zeroes;

  for (i = count; i >= PADSIZE; i -= PADSIZE)
    if (PUT (s, padptr, PADSIZE) != PADSIZE)
      return -1;
  if (i > 0)
    if (PUT (s, padptr, i) != i)
      return -1;

  return count;
}
#undef PADSIZE
#endif /* USE_IN_LIBIO */
