       #include <sys/socket.h>
       #include <netinet/in.h>
       #include <arpa/inet.h>


 int inet_xton(const char *cp, struct in_addr *inp)
{
	if (strcmp(cp, "255.255.255.255") == 0) {
		inp->s_addr = (unsigned) -1;
		return 1;
	}

	inp->s_addr = inet_addr(cp);
	if (inp->s_addr == (unsigned) -1) {
		return 0;
	}
	return 1;
}

main(int argc, char *argv[])
{
	struct in_addr ip;
	printf("%d\n", inet_xton(argv[1], &ip));
}
