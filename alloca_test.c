main()
{
	int i;
	for (i=0;i<100000;i++) {
		char *s = alloca(100000);
		memset(s, 0, 100000);
	}
	return 0;
}
