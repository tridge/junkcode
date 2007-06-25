#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>


main()
{
	printf("%d\n", sizeof(struct sockaddr_un));
}
