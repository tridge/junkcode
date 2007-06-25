#include <stdio.h>
#include "zlib.h"

int main(void)
{
	gzFile *g = gzdopen(1, "w");
	char buf[1024];
	int n;

	while ((n=read(0, buf, sizeof(buf))) > 0) {
		gzwrite(g, buf, n);
	}
	gzclose(g);
	return 0;
}
