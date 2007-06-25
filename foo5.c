
int sys_open(const char *fname, int opts)
{
	int ret;

	do {
		ret = open(fname, opts);
	} while (ret == -1 && errno == EINTR);
	return ret;
}

main()
{
	char *fname = "foo.txt";
	int fd;

again:
	fd = sys_open(fname, O_RDONLY);
	if (fd == -1) {
		if (errno == EINTR) goto again;
		perror(fname);
		exit(1);
	}
	

}
