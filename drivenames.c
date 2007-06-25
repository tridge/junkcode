
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

/*
  find a list of drive names on this machine
  return the number of drives found, return the names as 
  a string array in **names
*/
static int find_drive_names(char ***names)
{
	DIR *dir;
	struct dirent *de;
	int count = 0;

	dir = opendir("/proc/ide");
	if (!dir) {
		perror("/proc/ide");
		return 0;
	}

	(*names) = NULL;

	while ((de = (readdir(dir)))) {
		/* /proc/ide contains more than just a list of drives. We need
		   to select only those bits that match 'hd*' */
		if (strncmp(de->d_name, "hd", 2) == 0) {
			/* found one */
			(*names) = realloc(*names, sizeof(char *) * (count+2));
			if (! (*names)) {
				fprintf(stderr,"Out of memory in find_drive_names\n");
				return 0;
			}
			asprintf(&(*names)[count],"/dev/%s", de->d_name);
			count++;
		}
	}

	if (count != 0) {
		(*names)[count] = NULL;
	}
	
	closedir(dir);
	return count;
}

main()
{
	char **names;
	int n, i;

	n = find_drive_names(&names);

	for (i=0;i<n;i++) {
		printf("%s\n", names[i]);
	}
	
}
