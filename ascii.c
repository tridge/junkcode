typedef unsigned char uint8;

void blah(const uint8 *x)
{
	printf("%d\n", x[0]);
}

void main(void)
{
	blah((const uint8 *)"\04");
}
