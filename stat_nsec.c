#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/stat.h>

int main(int argc, char **argv)
{
	struct stat st;

	stat(argv[1], &st);
	printf("nsec=%lu\n", st.st_mtim.tv_nsec);
	return 0;
}
