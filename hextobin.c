/* convert a hex stream to a file.
   useful for reversing tcpdump or nc captures
*/
#include <stdio.h>

int main(void)
{
	int c;
	while (scanf("%2x", &c) == 1) {
		fputc(c, stdout);
	}
	return 0;
}
