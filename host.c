#include <stdio.h>
#include <unistd.h>

main()
{
	char buf[1024];
	gethostname(buf, sizeof(buf)-1);
	puts(buf);
}
