#include <stdio.h>

main()
{
	int c;
	while ((c=getchar()) != -1) {
		if (c == '\r') continue;
		putchar(c);
	}
}

