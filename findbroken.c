#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>


static void findit(char *dir)
{
	DIR *d;
	struct dirent *de;
	
	d = opendir(dir);
	if (!d) return;
	
	
	while ((de = readdir(d))) {
		char *fname;
		struct stat st, st2;
		
		if (strcmp(de->d_name,".")==0) continue;
		if (strcmp(de->d_name,"..")==0) continue;
		
		fname = (char *)malloc(strlen(dir) + strlen(de->d_name) + 2);
		if (!fname) {
			fprintf(stderr,"out of memory\n");
			exit(1);
		}
		sprintf(fname,"%s/%s", dir, de->d_name);
		
		if (lstat(fname, &st)) {
			perror(fname);
			continue;
		}
		
		if (S_ISLNK(st.st_mode) && 
		    (stat(fname,&st2) || st2.st_uid != st.st_uid)) {
			char buf[1024];
			readlink(fname, buf, sizeof(buf)-1);
			printf("%s -> %s\n", fname, buf);
		}
		
		if (S_ISDIR(st.st_mode)) {
			findit(fname);
		}
		
		free(fname);
	}
	
	closedir(d);
}


int main(int argc, char *argv[])
{
	int size;
	
	if (argc < 2) {
		fprintf(stderr,"%s: <dir>\n", argv[0]);
		exit(1);
	}
	
	findit(argv[1]);
	return 0;
}
