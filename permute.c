#include <stdio.h>

typedef unsigned long long uint64;

printbits(uint64 x)
{
	int i;
	for (i=0;i<64;i++) {
		if (x & (((uint64)1)<<i)) {
			printf("%d ", i);
		} 
	}
	printf("\n");
}

void permute(int n)
{
	uint64 x=0;

	if (n == 0) return;
	
	do {
		x++;
		printbits(x);
	} while (x != ((1<<n)-1));
}


int main(int argc, char *argv[])
{
	permute(atoi(argv[1]));
	return 0;
}
