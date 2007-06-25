#include <sys/stat.h>
#include <unistd.h>

main()
{
	struct stat st;
	stat("/dev/null", &st);
}
