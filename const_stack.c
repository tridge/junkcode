#include <stdio.h>


static void foo(const char *s)
{
	const char * const attrs[2] = { s, NULL };

	printf("%p\n", attrs);
	foo("blah");
}


int main(void)
{
	foo("foo");
}
