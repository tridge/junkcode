#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>

main()
{
	char buf[6] = "hello";
	snprintf(buf, 4, "help");
	printf("%s\n", buf);
}

