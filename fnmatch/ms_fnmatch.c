#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>

/*
  the original, recursive function. Needs replacing, but with exactly
  the same output
*/
static int fnmatch_orig(const char *p, const char *n)
{
	char c;

	while ((c = *p++)) {
		switch (c) {
		case '?':
			if (! *n) return -1;
			n++;
			break;

		case '>':
			if (n[0] == '.') {
				if (! n[1] && fnmatch_orig(p, n+1) == 0) return 0;
				if (fnmatch_orig(p, n) == 0) return 0;
				return -1;
			}
			if (! *n) return fnmatch_orig(p, n);
			n++;
			break;

		case '*':
			for (; *n; n++) {
				if (fnmatch_orig(p, n) == 0) return 0;
			}
			break;

		case '<':
			for (; *n; n++) {
				if (fnmatch_orig(p, n) == 0) return 0;
				if (*n == '.' && !strchr(n+1,'.')) {
					n++;
					break;
				}
			}
			break;

		case '"':
			if (*n == 0 && fnmatch_orig(p, n) == 0) return 0;
			if (*n != '.') return -1;
			n++;
			break;

		default:
			if (c != *n && toupper(c) != toupper(*n)) return -1;
			n++;
		}
	}
	
	if (! *n) return 0;
	
	return -1;
}


static int null_match(const char *p)
{
	for (;*p;p++) {
		if (*p != '*' &&
		    *p != '<' &&
		    *p != '"' &&
		    *p != '>') return -1;
	}
	return 0;
}

static int min_p_chars(const char *p)
{
	int ret;
	for (ret=0;*p;p++) {
		switch (*p) {
		case '*':
		case '<':
		case '>':
		case '"':
			break;
		case '?':
		default:
			ret++;
		}
	} 	
	return ret;
}

/*
  the original, recursive function. Needs replacing, but with exactly
  the same output
*/
static int fnmatch_test2(const char *p, const char *n)
{
	char c;

	if (min_p_chars(p) > strlen(n)) return -1;

//	printf("p=%s   n=%s\n", p, n);

	while ((c = *p++)) {
		switch (c) {
		case '?':
			if (! *n) {
				return -1;
			}
			n++;
			break;

		case '>':
			if (n[0] == '.') {
				if (! n[1] && null_match(p) == 0) {
					return 0;
				}
				break;
			}
			if (! *n) return null_match(p);
			n++;
			break;

		case '*':
			for (; *n; n++) {
				if (fnmatch_test2(p, n) == 0) {
					return 0;
				}
			}
			if (! *n) return null_match(p);
			return -1;

		case '<':
			for (; *n; n++) {
				if (fnmatch_test2(p, n) == 0) {
					return 0;
				}
				if (*n == '.' && 
				    !strchr(n+1,'.')) {
					n++;
					break;
				}
			}
			break;

		case '"':
			if (*n == 0 && 
			    null_match(p) == 0) {
				return 0;
			}
			if (*n != '.') return -1;
			n++;
			break;

		default:
			if (c != *n && 
			    toupper(c) != 
			    toupper(*n)) {
				return -1;
			}
			n++;
		}
	}
	
	if (! *n) {
		return 0;
	}
	
	return -1;
}

static char *compress_pattern(const char *pattern)
{
	char *p, *new;

	p = new = strdup(pattern);
	while (p[0] && p[1]) {
		/*  ** => *  */
		/*  *< => *  */
		if (p[0] == '*' && (p[1] == '*' || p[1] == '<'))
			memmove(p+1, p+2, strlen(p+1));
		/* << => <* */
		else if (p[0] == '<' && p[1] == '<')
			p[1] = '*';
		else
			p++;
	}
	return new;
}

/*
  the new, hopefully better function. Fiddle this until it works and is fast
*/
static int fnmatch_test(const char *p, const char *n)
{
	char *new = compress_pattern(p);
	int ret;

	ret = fnmatch_test2(new, n);
	free(new);
	return ret;
}

static void randstring(char *s, int len, const char *chars)
{
	while (len--) {
		*s++ = chars[random() % strlen(chars)];
	}
	*s = 0;
}

static const char *p_used;
static const char *n_used;

static void sig_alrm(int sig)
{
	printf("Too slow!!\np='%s'\ns='%s'\n", p_used, n_used);
	printf("compresed: '%s'\n", compress_pattern(p_used));
	exit(0);
}

static struct timeval tp1,tp2;

static void start_timer(void)
{
	gettimeofday(&tp1,NULL);
}

static double end_timer(void)
{
	gettimeofday(&tp2,NULL);
	return((tp2.tv_sec - tp1.tv_sec) + 
	       (tp2.tv_usec - tp1.tv_usec)*1.0e-6);
}

int main(void)
{
	int i;
	srandom(time(NULL));

	signal(SIGALRM, sig_alrm);

	start_timer();
	alarm(2);
	p_used = "*c*c*cc*c*c*c*c*cc*cc*cc*c*c*c*cc*c*cc*c*c*cc*cc*ccc*c*c*cc*c*c";
	n_used = "c.cccccccccccccccccccccccccccc";
	fnmatch_test(p_used, n_used);
	alarm(0);
	printf("took %.7f seconds\n", end_timer());

	for (i=0;i<100000;i++) {
		int len1 = random() % 20;
		int len2 = random() % 20;
		char *p = malloc(len1+1);
		char *n = malloc(len2+1);
		int ret1, ret2;

		randstring(p, len1, "*?a");
		randstring(n, len2, "a.");

		p_used = p;
		n_used = n;

		alarm(0);
		ret1 = fnmatch_orig(p, n);
		alarm(2);
		ret2 = fnmatch_test(p, n);
		alarm(0);

		if (ret1 != ret2) {
			printf("mismatch: ret1=%d ret2=%d pattern='%s' string='%s'\n",
			       ret1, ret2, p, n);
			printf("Pattern actually used: '%s' => '%s'\n",
			       p, compress_pattern(p));
			free(p);
			free(n);
			exit(0);
		}

		free(p);
		free(n);
		printf("%d\r", i);
		fflush(stdout);
	}

	printf("ALL OK\n");
	return 0;
}
