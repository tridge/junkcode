
static int xx(int *i)
{
	return 0;
}

struct {
	int (*fn)(void *);
} x;


int main(void)
{
	x.fn = xx;
	return 0;
}
