
typedef struct { unsigned x; } FOOBAR;

void bar(unsigned, FOOBAR, unsigned);

main()
{
	FOOBAR foo = (FOOBAR){3};
	bar(1, foo, 2);
}
