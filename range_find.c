#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/file.h>

static void *map_file(char *fname, size_t *size)
{
	int fd = open(fname, O_RDONLY);
	struct stat st;
	void *p;

	fstat(fd, &st);
	p = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);

	*size = st.st_size;
	return p;
}

int main(int argc, const char *argv[])
{
	const char *fname;
	char *p, *str;
	size_t size;
	int slen;

	if (argc < 3) {
		printf("range_find <file> <string>\n");
		exit(1);
	}

	fname = argv[1];
	str = argv[2];

	slen = strlen(str);

	p = map_file(fname, &size);
	if (p == NULL) {
		perror(fname);
	}
	
	printf("Searching for '%s' in '%s'\n", str, fname);


	return 0;
}
