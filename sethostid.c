#include <stdlib.h>
#include <stdio.h>


main(int argc, char *argv[])
{
	sethostid(strtol(argv[1], NULL, 0));
}
