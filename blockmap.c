#include <stdio.h>
#include <unistd.h>
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

static int block_match(unsigned char *p1, unsigned char *p2)
{
	int i;
	int ret=0;
	if (memcmp(p1, p2, 512) == 0) return 512;
	for (i=0;i<512;i++) if (p1[i] && p1[i] != 0xff && p1[i] == p2[i]) ret++;
	return ret;
}

static int best_block(char *p1, char *p2, int nblocks, int *x, int hint)
{
	int i;
	int besti = 0;
	int bestv = -1;
	for (i=0; i<nblocks && bestv < 512; i++) {
		int ri;
		int v;

		ri = hint + i;
		if (ri < nblocks) {
			v = block_match(p1, p2+ri*512);
			if (v > bestv) {
				besti = ri;
				bestv = v;
			}
		}

		ri = hint - i;
		if (ri >= 0) {
			v = block_match(p1, p2+ri*512);
			if (v > bestv) {
				besti = ri;
				bestv = v;
			}
		}
	}
	*x = bestv;
	return besti;
}

int main(int argc, char *argv[])
{
	size_t size1, size2;
	int blocks1, blocks2;
	char *p1, *p2;
	
	int i;
	static char zblock[512];

	p1 = map_file(argv[1], &size1);
	p2 = map_file(argv[2], &size2);
	
	blocks1 = size1>>9;
	blocks2 = size2>>9;
	
	for (i=0;i<blocks1;i++) {
		int j, v;
		if (memcmp(p1+i*512, zblock, 512) == 0) continue;
		j = best_block(p1+i*512, p2, blocks2, &v, i);
		printf("block %d best is %d\tv=%d\tdiff=%d\n", 
				   i, j, v, i-j);
	}
	return 0;
}
