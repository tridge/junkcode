#include <stdio.h>
#include <unistd.h>

main()
{
	int limit = __sysconf (_SC_NGROUPS_MAX);
	printf("Limit=%d\n", limit);
}
