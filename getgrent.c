#include <grp.h>
#include <sys/types.h>
#include <stdio.h>

main()
{
	struct group *grp;
	while (grp = getgrent()) {
		printf("%s : %d\n", grp->gr_name, (int)grp->gr_gid);
	}
	return 0;
}
