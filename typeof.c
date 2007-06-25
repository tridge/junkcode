
#define __UNSIGNED(x) ((unsigned typeof(x))(x))

static void foo(unsigned char *s)
{

}

int main(void)
{
	char *s = "foo";

	foo(s);

	foo((unsigned typeof(s)) s);

	return 0;
}
