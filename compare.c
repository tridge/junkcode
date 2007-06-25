#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int main(int argc,char *argv[])
{
	int ret;
	double n1, n2;
	if (argc < 3) {
		fprintf(stderr,"Usage: compare n1 n2\n");
		exit(1);
	}
	n1 = atof(argv[1]);
	n2 = atof(argv[2]);
	if (n1 > n2) {
		ret = 1;
	} else if (n1 < n2) {
		ret = -1;
	} else {
		ret = 0;
	}
	printf("%d\n", ret);
	return ret;
}
