#include <stdio.h>

typedef unsigned WORD;

WORD OSTranslate(
	WORD TranslateMode,
	char *In,
	WORD InLength,
	char *Out,
	WORD OutLength)
{
	printf("%s\n", In);
	return 0x42;
}


int main(void)
{
	OSTranslate(0, "foo", 3, "", 1);
	return 0;
}
