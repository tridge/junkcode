#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	if (argc < 2) {
		puts("crypt <password>\n");
		exit(1);
	}
	puts((char *)crypt(argv[1], "xx"));
	return 0;
}
