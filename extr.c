static char *extract_address(char *from)
{
        char *p, *q;
        p = strchr(from,'<');
        if (!p) return from;
        p++;
        q = strchr(p,'>');
        if (!q) return from;
        *q = 0;
        from = strdup(p);
        *q = '>';
        return from;
}

main(int argc, char *argv[])
{
	puts(extract_address(argv[1]));
}
