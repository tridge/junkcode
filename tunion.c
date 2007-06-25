#include <stdio.h>
#include <stdlib.h>

struct parm_struct
{
	char *label;
	union {
		int b;
		int i;
		char *s;
		char c;
	} u;
};


static struct parm_struct p[] =
{
  {"int", {i:42}},
  {"bool", {b:1}},
  {"string", {s:"foobar"}},
  {"char", {c:'t'}},
  {NULL}};

int main()
{
	printf("%d\n", p[0].u.i);
	printf("%d\n", p[1].u.b);
	printf("%s\n", p[2].u.s);
	printf("%c\n", p[3].u.c);
	return 0;
}
