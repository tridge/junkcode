#include <sys/param.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

 size_t strlcpy(char *d, const char *s, size_t bufsize)
{
	size_t len = strlen(s);
	size_t ret = len;
	if (bufsize <= 0) return 0;
	if (len >= bufsize) len = bufsize-1;
	memcpy(d, s, len);
	d[len] = 0;
	return ret;
}

static unsigned char read_byte(void)
{
	return 254;
}

main()
{
	char *dirname;
	int off;
	char *p;
	char *basename = "test";
	p = malloc(MAXPATHLEN);
	int len = read_byte();

	printf("len=%d\n", len);

	dirname = malloc(MAXPATHLEN+1);
	memset(dirname, 'x', MAXPATHLEN);
	dirname[MAXPATHLEN-2] = 0;

	off = strlcpy(p, dirname, MAXPATHLEN);
	printf("off=%d\n", off);
	off += strlcpy(p + off, "/", MAXPATHLEN - off);
	printf("off=%d\n", off);
	off += strlcpy(p + off, basename, MAXPATHLEN - off);
	printf("off=%d\n", off);

	printf("%d\n", MAXPATHLEN);
}
