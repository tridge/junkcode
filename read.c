int read_x(int fd, char *buf, size_t size)
{
	size_t n;
	if (size == 0) return 0;
	n = random() % size;
	if (n == 0) n = 1;
	return read(fd, buf, n);
}

foo()
{
	read_x(fd, buf, size);
}
