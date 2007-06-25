#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>



static int create_files(int dir, int nfiles)
{
	int i, fd;
	char *dname;
	asprintf(&dname, "dir%d", dir);

	if (mkdir(dname, 0755) != 0 && errno != EEXIST) {
		perror(dname);
		exit(1);
	}

	for (i=0;i<nfiles;i++) {
		char *fname;
		asprintf(&fname, "%s/File%d", dname, i);
		fd = open(fname, O_RDWR|O_CREAT|O_TRUNC, 0644);
		if (fd == -1) {
			perror(fname);
			exit(1);
		}
		write(fd, fname, strlen(fname)+1);
		close(fd);
		free(fname);
	}

	return 0;
}

static int check_files(int dir, int nfiles)
{
	int i, fd;
	char *dname;
	asprintf(&dname, "dir%d", dir);

	for (i=0;i<nfiles;i++) {
		char *fname;
		char s[100];
		asprintf(&fname, "%s/File%d", dname, i);
		fd = open(fname, O_RDONLY, 0644);
		if (fd == -1) {
			perror(fname);
			exit(1);
		}
		read(fd, s, sizeof(s));
		close(fd);

		if (strcmp(fname, s)) {
			printf("Name mismatch! %s %s\n", fname, s);
		}

		unlink(fname);
		free(fname);
	}

	if (rmdir(dname) != 0) {
		perror(dname);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int nprocs, nfiles, i, ret=0, status;

	nprocs = atoi(argv[1]);
	nfiles = atoi(argv[2]);

	printf("Creating %d files in %d dirs\n", nfiles, nprocs);

	for (i=0;i<nprocs;i++) {
		if (fork() == 0) {
			exit(create_files(i, nfiles));
		}
	}
	
	while (waitpid(0, &status, 0) > 0 || errno != ECHILD) {
		if (WEXITSTATUS(status) != 0) {
			ret = WEXITSTATUS(status);
			printf("Child exited with status %d\n", ret);
		}
	}

	for (i=0;i<nprocs;i++) {
		if (fork() == 0) {
			exit(check_files(i, nfiles));
		}
	}
	
	while (waitpid(0, &status, 0) > 0 || errno != ECHILD) {
		if (WEXITSTATUS(status) != 0) {
			ret = WEXITSTATUS(status);
			printf("Child exited with status %d\n", ret);
		}
	}

	return ret;
}
