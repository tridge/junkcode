#include <stdio.h>

int main(int argc, const char *argv[])
{
	FILE *f;
	char buf[8 * 1024 * 1024];

	f = fopen(argv[1], "r+");

	fwrite(buf, sizeof(buf), 1, f);

	sleep(100);
	return 0;
}
