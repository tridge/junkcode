#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
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

/*
  the original, recursive function. Needs replacing, but with exactly
  the same output
*/
static int fnmatch_test2(const char *p, size_t ofs, const char *n, unsigned *cache)
{
	char c;
	int len;

	if (strlen(n) == 1) {
//		printf("p=%s n=%s\n", p+ofs, n);
	}

//	printf("p=%s n=%s\n", p+ofs, n);
			

	while ((c = p[ofs++])) {
		switch (c) {
		case '*':
			len = strlen(n);
			for (; *n; n++) {
				if (cache[ofs] && cache[ofs] >= len) {
					return null_match(p+ofs);
				}
				if (fnmatch_test2(p, ofs, n, cache) == 0) {
					return 0;
				}
			}
			if (cache[ofs] < len) cache[ofs] = len;
			return null_match(p+ofs);

		case '<':
			for (; *n; n++) {
				if (fnmatch_test2(p, ofs, n, cache) == 0) {
					return 0;
				}
				if (*n == '.' && 
				    !strchr(n+1,'.')) {
					n++;
					break;
				}
			}
			break;


		case '?':
			if (! *n) {
				return -1;
			}
			n++;
			break;

		case '>':
			if (n[0] == '.') {
				if (! n[1] && null_match(p+ofs) == 0) {
					return 0;
				}
				break;
			}
			if (! *n) return null_match(p+ofs);
			n++;
			break;

		case '"':
			if (*n == 0 && 
			    null_match(p+ofs) == 0) {
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
			break;
		}
	}
	
	if (! *n) {
		return 0;
	}
	
	return -1;
}

/*
  the new, hopefully better function. Fiddle this until it works and is fast
*/
static int fnmatch_test(const char *p, const char *n)
{
	int ret;
	unsigned *cache;

	cache = calloc(sizeof(unsigned), strlen(p)+1);

	ret = fnmatch_test2(p, 0, n, cache);

	free(cache);

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
	double t1=0, t2=0;
	srandom(time(NULL));

	signal(SIGALRM, sig_alrm);

	start_timer();
	alarm(0);
	p_used ="*c*c*c";
	n_used = "ccc.";
	fnmatch_test(p_used, n_used);
	alarm(0);
	printf("took %.7f seconds\n", end_timer());
//	exit(0);

	for (i=0;i<100000;i++) {
		int len1 = random() % 35;
		int len2 = random() % 35;
		char *p = malloc(len1+1);
		char *n = malloc(len2+1);
		int ret1, ret2;

		randstring(p, len1, "*><\"?a.");
		randstring(n, len2, "a.");

		p_used = p;
		n_used = n;

		alarm(0);
		start_timer();
		ret1 = fnmatch_orig(p, n);
		t1 += end_timer();
		alarm(2);
		start_timer();
		ret2 = fnmatch_test(p, n);
		t2 += end_timer();
		alarm(0);

		if (ret1 != ret2) {
			printf("mismatch: ret1=%d ret2=%d pattern='%s' string='%s'\n",
			       ret1, ret2, p, n);
			free(p);
			free(n);
			exit(0);
		}

		free(p);
		free(n);
		printf("%d t1=%.5f  t2=%.5f\r", i, t1, t2);
		fflush(stdout);
	}

	printf("ALL OK t1=%.4f t2=%.4f\n", t1, t2);
	return 0;
}
