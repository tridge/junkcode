#include <stdio.h>
#include <mntent.h>

int main(void)
{
	FILE *f = setmntent("/etc/mtab", "r");
	struct mntent *m;

	while ((m = getmntent(f))) {
		printf("%s %s %s\n", 
		       m->mnt_fsname, m->mnt_dir, m->mnt_type);
	}
	
	endmntent(f);
	return 0;
}

