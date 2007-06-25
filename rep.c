#include <stdio.h>

short to=0x1043;
short from=0x1102;

main()
{
	int i, ofs=0, count=0;
	short x;
	while (!feof(stdin)) {
		fread(&x, sizeof(x), 1, stdin);
		if (x == from) {
			count++;
			if (count == 2) {
				x = to;
				fprintf(stderr,"changed at %d\n", ofs);
			}
		}
		fwrite(&x, sizeof(x), 1, stdout);
		ofs += 2;
	}
}
