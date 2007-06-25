#include <stdio.h>

struct user {
	char username[256];
	char groups[200][256];
};

void dans_func(IN/OUT STRING char x[100])
{
	char x[100];
	struct user users[MAX_USERS];


	strcpy(x, "foo");
	printf("%s\n", x);

	x[4] = '3';
}


main()
{
	dans_func();
}
