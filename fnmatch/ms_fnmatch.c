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


/*
  the new, hopefully better function. Fiddle this until it works and is fast
*/
static int fnmatch_test(const char *p, const char *n)
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
				if (! n[1] && fnmatch_test(p, n+1) == 0) return 0;
				if (fnmatch_test(p, n) == 0) return 0;
				return -1;
			}
			if (! *n) return fnmatch_test(p, n);
			n++;
			break;

		case '*':
			for (; *n; n++) {
				if (fnmatch_test(p, n) == 0) return 0;
			}
			break;

		case '<':
			for (; *n; n++) {
				if (fnmatch_test(p, n) == 0) return 0;
				if (*n == '.' && !strchr(n+1,'.')) {
					n++;
					break;
				}
			}
			break;

		case '"':
			if (*n == 0 && fnmatch_test(p, n) == 0) return 0;
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


static void randstring(char *s, int len, const char *chars)
{
	while (len--) {
		*s++ = chars[random() % strlen(chars)];
	}
	*s = 0;
}

static void sig_alrm(int sig)
{
	printf("Too slow!!\n");
	exit(0);
}

int main(void)
{
	int i;
	srandom(time(NULL));

	signal(SIGALRM, sig_alrm);

	alarm(2);
	fnmatch_test("********************************************.dat", "foobar.txt");
	fnmatch_test("*<*<*<*<*<*<*<*<*<*<*<*<*<*<*<*<*<*<*<*<*<*<.dat", "foobar.txt");
	alarm(0);

	for (i=0;i<100000;i++) {
		int len1 = random() % 20;
		int len2 = random() % 20;
		char *p = malloc(len1+1);
		char *n = malloc(len2+1);
		int ret1, ret2;

		randstring(p, len1, "*?<>\".abc");
		randstring(n, len2, "abc.");

		ret1 = fnmatch_orig(p, n);
		ret2 = fnmatch_test(p, n);

		if (ret1 != ret2) {
			printf("mismatch: ret1=%d ret2=%d pattern='%s' string='%s'\n",
			       ret1, ret2, p, n);
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
