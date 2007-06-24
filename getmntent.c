#include <stdio.h>
#include <mntent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void)
{
	FILE *f = setmntent("/etc/mtab", "r");
	struct mntent *m;

	while ((m = getmntent(f))) {
		struct stat st;
		dev_t dev = 0;
		if (stat(m->mnt_dir, &st) == 0) {
			dev = st.st_dev;
		}
		printf("%s %s %s 0x%llx\n", 
		       m->mnt_fsname, m->mnt_dir, m->mnt_type, (unsigned long long)dev);
	}
	
	endmntent(f);
	return 0;
}

