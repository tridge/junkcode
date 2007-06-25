#include <stdio.h>

typedef char fstring[128];

#define SIZEOF(x) ((sizeof(x)==4?nosuchfn():sizeof(x)))

void foo(fstring x)
{
	printf("sizeof x = %d\n", sizeof(x));
}

main()
{
	fstring x;
	printf("sizeof x = %d\n", sizeof(x));
	foo(x);
}
