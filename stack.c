
void r(int depth)
{
	int x = depth;
	printf("depth=%d &x=%p\n", depth, &x);

	if (depth == 0) return;
	r(depth-1);
}

main()
{
	r(100);
}
