#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>

static char *timestring(time_t t)
{
	static char TimeBuf[200];
	struct tm *tm = localtime(&t);

	strftime(TimeBuf,sizeof(TimeBuf)-1,"%Y/%m/%d %T",tm);

	if (TimeBuf[strlen(TimeBuf)-1] == '\n') {
		TimeBuf[strlen(TimeBuf)-1] = 0;
	}

	return(TimeBuf);
}

static void ls_one(char *fname)
{
	char perms[11] = "----------";
	char *perm_map = "rwxrwxrwx";
	struct stat st;
	int i;

	if (lstat(fname, &st) != 0) {
		perror(fname);
		return;
	}

	for (i=0;i<9;i++) {
		if (st.st_mode & (1<<i)) perms[9-i] = perm_map[8-i];
	}
	if (S_ISLNK(st.st_mode)) perms[0] = 'l';
	if (S_ISDIR(st.st_mode)) perms[0] = 'd';
	if (S_ISBLK(st.st_mode)) perms[0] = 'b';
	if (S_ISCHR(st.st_mode)) perms[0] = 'c';
	if (S_ISSOCK(st.st_mode)) perms[0] = 's';
	if (S_ISFIFO(st.st_mode)) perms[0] = 'p';


	printf("%s %11.0f %s %s\n", 
	       perms, 
	       (double)st.st_size, timestring(st.st_mtime), fname);
}

int main(int argc, char *argv[])
{
	int i;

	for (i=1; i<argc;i++) {
		ls_one(argv[i]);
	}
	return 0;
}
