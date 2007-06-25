#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

 int setenv(const char *name, const char *value, int overwrite) 
{
	char *p;
	size_t l1, l2;
	int ret;

	if (!overwrite && getenv(name)) {
		return 0;
	}

	l1 = strlen(name);
	l2 = strlen(value);

	p = malloc(l1+l2+2);
	if (p == NULL) {
		return -1;
	}
	memcpy(p, name, l1);
	p[l1] = '=';
	memcpy(p+l1+1, value, l2);
	p[l1+l2+1] = 0;

	ret = putenv(p);
	if (ret != 0) {
		free(p);
	}

	return ret;
}


int main(int argc, char *argv[])
{
	int ret1, ret2;

	ret1 = setenv("foo", "blah", 0);
	ret2 = setenv("foo", "blah2", 0);
	printf("ret1=%d ret2=%d '%s'\n", ret1, ret2, getenv("foo"));
	return 0;
}
