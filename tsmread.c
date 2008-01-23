/*
  example program to read a file in the same way that Samba does. This
  demonstrates the problem with GPFS share modes and leases and TSM
  migrated files

  Andrew Tridgell January 2008

  compile with:

     gcc -Wall -o tsmread{,.c} -lgpfs_gpl

  then to run you must symlink tsmread to smbd

     ln -s tsmread smbd

  and run like this:

    ./smbd /gpfs/data/tsmtest/test.dat 
    ./smbd /gpfs/data/tsmtest/test.dat -S
    ./smbd /gpfs/data/tsmtest/test.dat -L
    ./smbd /gpfs/data/tsmtest/test.dat -L -S

 */

#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include "gpfs_gpl.h"


static void sigio_handler(int sig)
{
	printf("Got SIGIO\n");
}

static int read_file(const char *fname, bool use_lease, bool use_sharemode)
{
	int fd;
	char c;

	printf("Reading '%s' with use_lease=%s use_sharemode=%s\n",
	       fname, use_lease?"yes":"no", use_sharemode?"yes":"no");

	signal(SIGIO, sigio_handler);

	fd = open(fname, O_RDONLY);
	if (fd == -1) {
		perror(fname);
		return -1;
	}

	if (use_lease && gpfs_set_lease(fd, GPFS_LEASE_READ) != 0) {
		perror("gpfs_set_lease");
		close(fd);
		return -1;
	}

	if (use_sharemode && gpfs_set_share(fd, 1, 2) != 0) {
		perror("gpfs_set_share");
		close(fd);
		return -1;
	}

	if (pread(fd, &c, 1, 0) != 1) {
		perror("pread");
		close(fd);
		return -1;		
	}

	printf("read OK\n");
	
	close(fd);
	return 0;	
}

static void usage(void)
{
	printf("Usage: (note, must run as 'smbd')\n");
	printf("ln -sf tesmread smbd\n");
	printf("./smbd [options] <files>\n");
	printf("Options:\n");
	printf("  -L    use gpfs leases\n");
	printf("  -S    use gpfs sharemodes\n");
	exit(0);
}

int main(int argc, char * const argv[])
{
	int opt, i;
	bool use_lease = false, use_sharemode = false;
	
	/* parse command-line options */
	while ((opt = getopt(argc, argv, "LSh")) != -1) {
		switch (opt){
		case 'L':
			use_lease = true;
			break;
		case 'S':
			use_sharemode = true;
			break;
		default:
			usage();
			break;
		}
	}

	argv += optind;
	argc -= optind;

	if (argc == 0) {
		usage();
	}

	for (i=0;i<argc;i++) {
		const char *fname = argv[i];
		if (read_file(fname, use_lease, use_sharemode) != 0) {
			printf("Failed to read '%s'\n", fname);
		}
	}

	return 0;
}
