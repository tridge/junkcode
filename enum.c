#include <stdio.h>

enum xxy;

typedef struct { int v; } BOOL;
typedef enum {NO_PROBLEM=0, BLAH=1} NT_STATUS;
#define False 0

static BOOL foo1(void)
{
	return False;
}

static void foo2(NT_STATUS x)
{
	printf("x=%d\n", x);
}

int main(void)
{
	foo2(foo1());
	return 0;
}
