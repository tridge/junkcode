int foo(int y)
{
	int x[3];

	printf("x2=%d\n", x[2]);

	x[3] = 0;
}

main()
{
	foo(0);
}
