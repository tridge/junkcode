#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>


static struct timeval tp1,tp2;

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


static void read_file(char *fname)
{
	int fd;
	static double total, thisrun;
	int n;
	char buf[0x10000];

	fd = open(fname, O_RDONLY);
	if (fd == -1) {
		perror(fname);
		return;
	}

	while ((n = read(fd, buf, sizeof(buf))) > 0) {
		total += n;
		thisrun += n;
		if (end_timer() >= 2.0) {
			printf("%d MB    %g MB/sec\n", 
			       (int)(total/1.0e6),
			       (thisrun*1.0e-6)/end_timer());
			start_timer();
			thisrun = 0;
		}
	}

	close(fd);
}


int main(int argc, char *argv[])
{
	int i;

	start_timer();

	while (1) {
		for (i=1; i<argc; i++) {
			read_file(argv[i]);
		}
	}

	return 0;
}

