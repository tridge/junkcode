#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>
#include <sys/mount.h>
#include <sys/time.h>

_syscall5(int , _llseek, 
	  int, fd, 
	  unsigned long , offset_high,
	  unsigned long , offset_low,
	  loff_t *, result,
	  unsigned int, origin)

typedef unsigned long long uint64;

static void do_seek(int fd, uint64 ofs)
{
	int ret;
	unsigned offset1, offset2;
	uint64 result = 0;

	offset1 = ofs>>32;
	offset2 = ofs&0xFFFFFFFF;

	ret = _llseek(fd, offset1, offset2, &result, SEEK_SET);

	if (result != ((((long long)offset1) << 32) | offset2)) {
		printf("llseek failed %d\n", ret);
		exit(1);
	}
}

static void init_buf(unsigned *buf, int seed)
{
	int i;
	srandom(seed);
	for (i=0;i<1024/4;i++) buf[i] = random();
}

static void write_block(int fd, unsigned *buf, int i)
{
	do_seek(fd, i*(uint64)1024);
	if (write(fd, buf, 1024) != 1024) {
		fprintf(stderr,"write failed on block %d\n", i);
		exit(1);
	}
}

static void check_block(int fd, unsigned *buf, int i)
{
	unsigned buf2[1024/4];

	do_seek(fd, i*(uint64)1024);
	if (read(fd, buf2, 1024) != 1024) {
		fprintf(stderr,"read failed on block %d\n", i);
		exit(1);
	}
	if (memcmp(buf, buf2, 1024) != 0) {
		int j, count=0;
		for (j=0; j<1024/4; j++) {
			if (buf[j] != buf2[j]) count++;
		}
		printf("compare failed on block %d (%d words differ)\n", i, count);
		fd = open("buf1.dat", O_WRONLY|O_CREAT|O_TRUNC, 0644);
		write(fd, buf, 1024);
		close(fd);
		fd = open("buf2.dat", O_WRONLY|O_CREAT|O_TRUNC, 0644);
		write(fd, buf2, 1024);
		close(fd);
		printf("Wrote good data to buf1.dat\n");
		printf("Wrote bad data to buf2.dat\n");
		exit(1);
	}
}

static void disk_test(char *dev)
{
	int fd;
	int blocks, i, count=0;
	int num_reads = 0;
	int num_writes = 0;
	unsigned *seeds;
	unsigned *buf;

	fd = open(dev, O_RDWR);
	if (fd == -1) {
		perror(dev);
		return;
	}

	if (ioctl(fd, BLKGETSIZE, &blocks) != 0) {
		fprintf(stderr,"BLKGETSIZE failed\n");
		exit(1);
	}

	blocks >>= 1;
	printf("%s is %d blocks long\n", dev, blocks);

	seeds = calloc(blocks, sizeof(unsigned));
	if (!seeds) {
		fprintf(stderr,"malloc of seeds failed\n");
		exit(1);
	}

	buf = (unsigned *)memalign(1024, 1024);

	srandom(time(NULL));

	while (1) {
		i = ((unsigned)random()) % blocks;

		if (seeds[i] == 0 || (random() & 2)) {
			seeds[i] = random();
			init_buf(buf, seeds[i]);
			write_block(fd, buf, i);
			num_writes++;
		} else {
			init_buf(buf, seeds[i]);
			check_block(fd, buf, i);
			num_reads++;
		}
		count++;
		if (count % 500 == 0) {
			printf("%d blocks (%d reads, %d writes)\n", 
			       count, num_reads, num_writes);
			srandom(random() | time(NULL));
		}
	}

	close(fd);
}


int main(int argc, char *argv[])
{
	char *dev;

	if (argc < 2) {
		fprintf(stderr,"usage: disktest <device>\n");
		exit(1);
	}

	dev = argv[1];

	disk_test(dev);

	return 0;
}

