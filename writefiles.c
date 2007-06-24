#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>


static struct timeval tp1,tp2;
static size_t block_size = 64 * 1024;
static int osync;

static void start_timer()
{
	gettimeofday(&tp1,NULL);
}

static double end_timer()
{
	gettimeofday(&tp2,NULL);
	return((tp2.tv_sec - tp1.tv_sec) + 
	       (tp2.tv_usec - tp1.tv_usec)*1.0e-6);
}


static void write_file(char *fname)
{
	int fd;
	static double total, thisrun;
	int n;
	char *buf;

	buf = malloc(block_size);

	if (!buf) {
		printf("Malloc of %d failed\n", (int)block_size);
		exit(1);
	}

	fd = open(fname, O_WRONLY|(osync?O_SYNC:0));
	if (fd == -1) {
		perror(fname);
		free(buf);
		return;
	}

	while ((n = write(fd, buf, block_size)) > 0) {
		total += n;
		thisrun += n;
		if (end_timer() >= 1.0) {
			time_t t = time(NULL);
			printf("%6d MB    %.3f MB/sec  %s", 
			       (int)(total/1.0e6),
			       (thisrun*1.0e-6)/end_timer(),
			       ctime(&t));
			start_timer();
			thisrun = 0;
		}
	}

	free(buf);
	close(fd);
}


static void usage(void)
{
	printf("\n" \
"writefiles - writes to a list of files, showing throughput\n" \
"\n" \
"Usage: writefiles [options] <files>\n" \
"\n" \
"Options:\n" \
"    -B size        set the block size in bytes\n" \
"    -s             sync open\n" \
"\n" \
"WARNING: writefiles is a destructive test!\n" \
"\n" \
"");
}


int main(int argc, char *argv[])
{
	int i;
	extern char *optarg;
	extern int optind;
	int c;

	while ((c = getopt(argc, argv, "B:hs")) != -1) {
		switch (c) {
		case 'B':
			block_size = strtol(optarg, NULL, 0);
			break;
		case 's':
			osync = 1;
			break;
		case 'h':
		default:
			usage();
			exit(1);
		}
	}

	argc -= optind;
	argv += optind;

	if (argc == 0) {
		usage();
		exit(1);
	}


	start_timer();

	while (1) {
		for (i=0; i<argc; i++) {
			write_file(argv[i]);
		}
	}

	return 0;
}

