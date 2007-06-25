#include <stdio.h>
#include <sys/types.h>
#include <utime.h>

int main(int argc, char *argv[])
{
	return utime(argv[1], NULL);
}
