int checkcode(char *code)
{
	int i, sum;

	sum = 0;

	for (i=0;code[i];i++) {
		sum += i%2==0 ? 3*code[i] : 2*code[i];
	}
	return sum % 10;
}

main(int argc, char *argv[])
{
	printf("%s: %d\n", argv[1], checkcode(argv[1]));
}
