void foo(int *x)
{
	*x = 2;
}

int main(void)
{
	foo((int *)3);
	return 0;
}
