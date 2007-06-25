main()
{
	char buf[1024];
	sprintf(buf, "Rhosts authentication refused for %.100: no home directory %.200s", "hello", "goodbye");

	printf(buf);
}
