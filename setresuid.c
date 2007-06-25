#include <errno.h>

main()
{
	setresuid(1,1,1);
	setresuid(2,2,2);
	if (errno != EPERM) {
		printf("no setresuid\n");
	}
}
