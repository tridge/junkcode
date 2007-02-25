/*
  demonstration of a memory leak in the AIX C library UESS subsystem

  tridge@au.ibm.com, January 2004
*/

#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <malloc.h>

/*
  return the current size of the heap (in blocks)
*/
static unsigned long heap_size(void)
{
	struct mallinfo m = mallinfo();
	return m.ordblks + m.smblks;
}


int main(int argc, char *argv[])
{
	char *name;
	int loops=0;

	if (argc < 2) {
		printf("usage: uess_leak <USERNAME>\n");
		exit(1);
	}

	name = argv[1];

	while (1) {
		struct passwd *pwd = getpwnam(name);
		if (!pwd) {
			perror("getpwnam");
			exit(1);
		}
		printf("memory blocks used %ld after %d calls\r", 
		       heap_size(), loops++);
		fflush(stdout);
	}
	return 0;
}
