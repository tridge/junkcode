#include <stdio.h>

int main(void)
{
	FILE *f = fopen("foo", "w");
	fclose(f);
	return 1;
}
