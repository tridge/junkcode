#include "rproxy.h"

static struct timeval tp1,tp2;

void start_timer()
{
	gettimeofday(&tp1,NULL);
}

double end_timer()
{
	gettimeofday(&tp2,NULL);
	return((tp2.tv_sec - tp1.tv_sec) + 
	       (tp2.tv_usec - tp1.tv_usec)*1.0e-6);
}


/* like fgets but strip off trailing \n and \r */
char *fgets_strip(char *s, int size, FILE *f)
{
	int l;

	if (!fgets(s, size, f)) return NULL;
	l = strlen(s);
	while (l>0 && (s[l-1] == '\n' || s[l-1] == '\r')) l--;
	s[l] = 0;
	return s;
}


/* dup a string and exit if it fails */
char *xstrdup(char *s)
{
	s = strdup(s);
	if (!s) {
		fprintf(stderr,"Out of memory in xstrdup\n");
		exit(1);
	}
	return s;
}

/* realloc some memory exit if it fails */
void *xrealloc(void *p, size_t length)
{
	if (!p) return xmalloc(length);
	p = realloc(p, length);
	if (!p) {
		fprintf(stderr,"Out of memory in xrealloc\n");
		exit(1);
	}
	return p;
}


/* malloc some memory exit if it fails */
void *xmalloc(size_t length)
{
	void *p = malloc(length);
	if (!p) {
		fprintf(stderr,"Out of memory in xmalloc\n");
		exit(1);
	}
	return p;
}

FILE *xfdopen(int fd, char *mode)
{
	FILE *f = fdopen(fd, mode);
	if (!f) {
		fprintf(stderr, "fdopen failed\n");
		exit(1);
	}
	return f;
}

/* read data form  stream, looping till all data is read */
ssize_t read_all(FILE *f, char *buf, size_t size)
{
	ssize_t total=0;
	while (size) {
		ssize_t n = fread(buf, 1, size, f);
		if (n == -1) return -1;
		if (n == 0) break;
		size -= n;
		buf += n;
		total += n;
	}
	return total;
}


/***************************************************************************
decode a base64 string in-place - simple and slow algorithm
  ***************************************************************************/
size_t base64_decode(char *s)
{
	char *b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int bit_offset, byte_offset, idx, i, n;
	unsigned char *d = (unsigned char *)s;
	char *p;

	n=i=0;

	while (*s && (p=strchr(b64,*s))) {
		idx = (int)(p - b64);
		byte_offset = (i*6)/8;
		bit_offset = (i*6)%8;
		d[byte_offset] &= ~((1<<(8-bit_offset))-1);
		if (bit_offset < 3) {
			d[byte_offset] |= (idx << (2-bit_offset));
			n = byte_offset+1;
		} else {
			d[byte_offset] |= (idx >> (bit_offset-2));
			d[byte_offset+1] = 0;
			d[byte_offset+1] |= (idx << (8-(bit_offset-2))) & 0xFF;
			n = byte_offset+2;
		}
		s++; i++;
	}

	return n;
}

/***************************************************************************
encode a buffer as base64 - simple and slow algorithm
  ***************************************************************************/
void base64_encode(unsigned char *buf, int n, char *out)
{
	char *b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int bytes, i;

	/* work out how many bytes of output there are */
	bytes = ((n*8)+5)/6;

	for (i=0;i<bytes;i++) {
		int byte = (i*6)/8;
		int bit = (i*6)%8;
		if (bit < 3) {
			if (byte >= n) abort();
			*out = b64[(buf[byte] >> (2-bit)) & 0x3F];
		} else {
			if (byte+1 == n) {
				*out = b64[(buf[byte] << (bit-2)) & 0x3F];
			} else {
				*out = b64[(buf[byte] << (bit-2) | 
					    buf[byte+1] >> (10 - bit)) & 0x3F];
			}
		}
		out++;
	}
	*out = 0;
}


/* this is like vsnprintf but it always null terminates, so you
   can fit at most n-1 chars in */
int vslprintf(char *str, int n, const char *format, va_list ap)
{
	int ret = vsnprintf(str, n, format, ap);
	if (ret >= n || ret < 0) {
		str[n-1] = 0;
		return -1;
	}
	str[ret] = 0;
	return ret;
}


/* like snprintf but always null terminates */
int slprintf(char *str, int n, char *format, ...)
{
	va_list ap;  
	int ret;

	va_start(ap, format);
	ret = vslprintf(str,n,format,ap);
	va_end(ap);
	return ret;
}


/****************************************************************************
  Get the next token from a string, return False if none found
  handles double-quotes. 
Based on a routine by GJC@VILLAGE.COM. 
Extensively modified by Andrew.Tridgell@anu.edu.au
****************************************************************************/
BOOL next_token(char **ptr,char *buff,char *sep)
{
  char *s;
  BOOL quoted;
  static char *last_ptr=NULL;

  if (!ptr) ptr = &last_ptr;
  if (!ptr) return(False);

  s = *ptr;

  /* default to simple separators */
  if (!sep) sep = " \t\n\r";

  /* find the first non sep char */
  while(*s && strchr(sep,*s)) s++;

  /* nothing left? */
  if (! *s) return(False);

  /* copy over the token */
  for (quoted = False; *s && (quoted || !strchr(sep,*s)); s++)
    {
      if (*s == '\"') 
	quoted = !quoted;
      else
	*buff++ = *s;
    }

  *ptr = (*s) ? s+1 : s;  
  *buff = 0;
  last_ptr = *ptr;

  return(True);
}


/****************************************************************************
substitute one substring for another in a string
****************************************************************************/
void string_sub(char *s,const char *pattern,const char *insert)
{
	char *p;
	size_t ls,lp,li;

	if (!insert || !pattern || !s) return;

	ls = strlen(s);
	lp = strlen(pattern);
	li = strlen(insert);

	if (!*pattern) return;
	
	while (lp <= ls && (p = strstr(s,pattern))) {
		memmove(p+li,p+lp,ls + 1 - (PTR_DIFF(p,s) + lp));
		memcpy(p, insert, li);
		s = p + li;
		ls += (li-lp);
	}
}


/*******************************************************************
trim the specified elements off the front and back of a string
********************************************************************/
BOOL trim_string(char *s,const char *front,const char *back)
{
	BOOL ret = False;
	size_t front_len = (front && *front) ? strlen(front) : 0;
	size_t back_len = (back && *back) ? strlen(back) : 0;
	size_t s_len;

	while (front_len && strncmp(s, front, front_len) == 0) {
		char *p = s;
		ret = True;
		while (1) {
			if (!(*p = p[front_len]))
				break;
			p++;
		}
	}

	if (back_len) {
		s_len = strlen(s);
		while ((s_len >= back_len) && 
		       (strncmp(s + s_len - back_len, back, back_len)==0)) {
			ret = True;
			s[s_len - back_len] = '\0';
			s_len = strlen(s);
		}
	} 
	
	return(ret);
}
