#define xx(y) char *y; printf("y");

#define stringize(x) #x

#define FOO 3

#define MYASSERT(x) do { if (x) { printf("Assert failed '%s'\n", #x); }} while (0)

main()
{
	MYASSERT(x > y);
}
