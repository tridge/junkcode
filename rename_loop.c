#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

static struct timeval tp1,tp2;

void start_timer()
{
	gettimeofday(&tp1,NULL);
}

double end_timer()
{
	gettimeofday(&tp2,NULL);
	return((tp2.tv_sec - tp1.tv_sec) + 
	       (tp2.tv_usec - tp1.tv_usec)*1.0e-6);
}

int main(void)
{
	const char *name1 = "test1.dat";
	const char *name2 = "test2.dat";
	int fd, ops;

	fd = open(name1, O_CREAT|O_RDWR|O_TRUNC, 0644);
	if (fd == -1) {
		perror("open");
		exit(1);
	}
	close(fd);

	start_timer();

	ops = 0;

	while (1) {
		if (rename(name1, name2) != 0 ||
		    rename(name2, name1) != 0) {
			perror("rename");
			exit(1);
		}
		ops++;
		if (end_timer() >= 1.0) {
			printf("%.1f ops/sec\n", (2*ops) / end_timer());
			ops = 0;
			start_timer();
		}
	}
	
	return 0;
}
