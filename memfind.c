#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

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

int main(int argc, char *argv[])
{
	size_t size1, size2, ofs1=0;
	char *p1, *p2;

	p1 = map_file(argv[1], &size1);
	p2 = map_file(argv[2], &size2);
	
	while (ofs1 < size1) {
		int n = 32;
		char *q, *p = memmem(p2, size2, p1+ofs1, 32);
		if (!p) {
			printf("data at %d not found!\n", ofs1);
			exit(1);
		}
		
		while (p[n] == p1[ofs1+n]) n++;
		printf("found 0x%x bytes at 0x%x (to 0x%x)\n",
		       n, (int)(p-p2), (int)(n+(p-p2)));
		ofs1 += n;
	}
}
