#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define GZIP_HEADER "\037\213\008"

static void *map_file(const char *fname, off_t *size)
{
	int fd;
	struct stat st;
	void *p;

	fd = open(fname, O_RDONLY);
	if (fd == -1) {
		perror(fname);
		return NULL;
	}

	fstat(fd, &st);
	p = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);

	*size = st.st_size;
	return p;
}


static void file_save(const char *fname, const char *p, off_t size)
{
	int fd;
	fd = open(fname, O_CREAT|O_TRUNC|O_WRONLY, 0666);
	if (fd == -1) {
		perror(fname);
		return;
	}
	if (write(fd, p, size) != size) {
		fprintf(stderr, "Failed to save %d bytes to %s\n", (int)size, fname);
		return;
	}
	close(fd);
}

static void scan_memory(int i, unsigned char *p, off_t size)
{
	unsigned char *p0 = p;
	int found = 0;
	
	while ((p = memmem(p, size - (p-p0), GZIP_HEADER, strlen(GZIP_HEADER)))) {
		char *fname=NULL;

		if (p[8] > 4) {
			p++;
			continue;
		}

		/* only want unix gzip output */
		if (p[9] > 11) {
			p++;
			continue;
		}

		found++;
		asprintf(&fname, "extract-%d-%d.gz", i, found);
		file_save(fname, p, size - (p-p0));
		printf("Extracted %d bytes to %s\n", (int)(size - (p-p0)), fname);
		free(fname);
		p++;
	}
}

int main(int argc, const char *argv[])
{
	int i;

	for (i=1;i<argc;i++) {
		unsigned char *p;
		off_t size;

		printf("Scanning %s\n", argv[i]);

		p = map_file(argv[i], &size);
		if (p) {
			scan_memory(i, p, size);
		}
	}

	return 0;
}
