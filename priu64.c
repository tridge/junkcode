#include <stdio.h>
#include <inttypes.h>

int main(void)
{
	char x[100];
	sprintf(x, "%"PRIu64, (unsigned long long)20);
	if (strcmp(x, "20") != 0) {
		printf("FAILED '%s'\n", x);
		exit(1);
	}
	return 0;
}
