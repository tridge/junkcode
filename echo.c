#include <stdio.h>


#define MIN(a,b) ((a)<(b)?(a):(b))


static void foo(char *inbuf)
{
	unsigned int data_len = 0x10000 - 39;

	data_len = MIN(data_len, (sizeof(inbuf)-(37)));

	printf("data_len=%u\n", data_len);
}

int main(void)
{
	char buf[0x10000];

	foo(buf);
}
