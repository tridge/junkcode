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


struct max_n {
	const char *predot;
	const char *postdot;
};

/*
  the test function
*/
static int fnmatch_test2(const char *p, const char *n, struct max_n *max_n, const char *ldot)
{
	char c;
	int i;

	while ((c = *p++)) {
		switch (c) {
		case '*':
			if (max_n->predot && max_n->predot <= n) {
				return null_match(p);
			}
			for (i=0; n[i]; i++) {
				if (fnmatch_test2(p, n+i, max_n+1, ldot) == 0) {
					return 0;
				}
			}
			if (!max_n->predot || max_n->predot > n) max_n->predot = n;
			return null_match(p);

		case '<':
			if (max_n->predot && max_n->predot <= n) {
				return null_match(p);
			}
			if (max_n->postdot && max_n->postdot <= n && n < ldot) {
				return -1;
			}
			for (i=0; n[i]; i++) {
				if (fnmatch_test2(p, n+i, max_n+1, ldot) == 0) return 0;
				if (n+i == ldot) {
					if (fnmatch_test2(p, n+i+1, max_n+1, ldot) == 0) return 0;
					if (!max_n->postdot || max_n->postdot > n) max_n->postdot = n;
					return -1;
				}
			}
			if (!max_n->predot || max_n->predot > n) max_n->predot = n;
			return null_match(p);

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

		case '"':
			if (*n == 0 && null_match(p) == 0) {
				return 0;
			}
			if (*n != '.') return -1;
			n++;
			break;

		default:
			if (c != *n && toupper(c) != toupper(*n)) {
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
	int ret, count, i;
	struct max_n *max_n = NULL;

	for (count=i=0;p[i];i++) {
		if (p[i] == '*' || p[i] == '<') count++;
	}

	if (count) {
		max_n = calloc(sizeof(struct max_n), count);
	}

	ret = fnmatch_test2(p, n, max_n, strrchr(n, '.'));

	if (max_n) {
		free(max_n);
	}

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
	printf("\nToo slow!!\np='%s'\ns='%s'\n", p_used, n_used);
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
	double t1, w1=0, t2, w2=0;
	srandom(time(NULL));

	signal(SIGALRM, sig_alrm);

	for (i=0;i<200000;i++) {
		int len1 = random() % 25;
		int len2 = random() % 25;
		char *p = malloc(len1+1);
		char *n = malloc(len2+1);
		int ret1, ret2;

		randstring(p, len1, "*<>\"?a.");
//		randstring(p, len1, "<a.");
		randstring(n, len2, "a.");

		p_used = p;
		n_used = n;

		alarm(2);
		start_timer();
		ret1 = ret2 = fnmatch_test(p, n);
		t2 = end_timer();
		alarm(0);

		alarm(0);
		start_timer();
		ret1 = fnmatch_orig(p, n);
		t1 = end_timer();

		if (t1 > w1) w1 = t1;
		if (t2 > w2) w2 = t2;

		if (ret1 != ret2) {
			printf("\nmismatch: ret1=%d ret2=%d pattern='%s' string='%s'\n",
			       ret1, ret2, p, n);
			free(p);
			free(n);
			exit(0);
		}

		free(p);
		free(n);
		printf("%7d worst1=%2.5f  worst2=%2.5f  (ratio=%5.2f)\r", 
		       i, w1, w2, w1/w2);
		fflush(stdout);
	}

	printf("\nALL OK speedup=%.4f\n", w1/w2);
	return 0;
}
