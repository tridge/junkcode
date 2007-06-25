static void foo1(void)
{
	char *p = alloca(32);
	p[0] = 0x42;
}

main()
{
	foo1();
}
