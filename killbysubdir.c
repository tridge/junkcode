#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

int main(int argc, char *argv[])
{
	char *directory;
	DIR *d;
	struct dirent *de;
	char buf[PATH_MAX];
	size_t directory_len;

	if (argc < 2) {
		fprintf(stderr,"%s: <directory>\n", argv[0]);
		exit(1);
	}
	
	directory = argv[1];

	/* make it absolute */
	if (directory[0] != '/') {
		char *cwd = getcwd(buf, sizeof(buf));
		if (cwd == NULL) {
			perror("cwd");
			exit(1);
		}
		asprintf(&directory, "%s/%s", cwd, directory);
	}

	/* resolve links etc */
	directory = realpath(directory, buf);

	if (directory == NULL) {
		perror("realpath");
		exit(1);
	}

	directory_len = strlen(directory);
	
	d = opendir("/proc");
	if (d == NULL) {
		perror("/proc");
		exit(1);
	}

	while ((de = readdir(d))) {
		const char *name = de->d_name;
		char *cwd_path, *real_cwd;
		char cwd[PATH_MAX], buf2[PATH_MAX];
		ssize_t link_size;

		if (!isdigit(name[0])) continue;
		asprintf(&cwd_path, "/proc/%s/cwd", name);
		link_size = readlink(cwd_path, cwd, sizeof(cwd));
		free(cwd_path);
		if (link_size == -1 || link_size >= sizeof(cwd)) {
			continue;
		}

		real_cwd = realpath(cwd, buf2);
		if (real_cwd == NULL) {
			continue;
		}

		if (strncmp(directory, real_cwd, directory_len) == 0 &&
		    (real_cwd[directory_len] == 0 || real_cwd[directory_len] == '/')) {
			/* kill it! */
			printf("Killing process %s\n", name);
			kill(atoi(name), SIGKILL);
		}
		
	}
	
	return 0;
}
