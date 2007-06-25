struct foo {
	char b[1];
	double x;
};


main()
{
	char buf[1024];
	struct foo *f;
	f = (struct foo *)&buf[3];
	printf("%d\n", (&f.x) - (&f.b));
}
