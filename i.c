/* a surprising useful little program! (tridge@samba.org) */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

static void print_one(unsigned long long v) 
{
	printf("%llu 0x%llX 0%llo", v, v, v);
	if (v < 256 && isprint(v)) printf(" '%c'", (char)v);
	printf("\n");
}

int main(int argc,char *argv[])
{
	int i;
	for (i=1;i<argc;i++) {
		char *p;
		unsigned long long v = strtoll(argv[i], &p, 0);
		if (p == argv[i]) for (; *p ; p++) print_one(*p);
		else print_one(v);
	}
	return 0;
}
