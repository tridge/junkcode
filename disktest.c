#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <asm/unistd.h>
#include <asm/fcntl.h>
#include <sys/mount.h>
#include <sys/time.h>

static int blocks;
static int block_size = 1024;
static unsigned read_prob = 50;
static unsigned num_reads, num_writes, num_fails;
static unsigned rep_time = 10;

void bit_set(unsigned char *bm, off_t i)
{
	bm[i/8] |= (1<<(i%8));
}

unsigned bit_query(unsigned char *bm, off_t i)
{
	return  (bm[i/8] & (1<<(i%8))) ? 1 : 0;
}

static void do_seek(int fd, off_t ofs)
{
	if (lseek(fd, ofs, SEEK_SET) != ofs) {
		perror("lseek");
		exit(1);
	}
}

static void init_buf(unsigned *buf, unsigned bnum)
{
	unsigned i;
	srandom(bnum);
	buf[0] = bnum;
	for (i=1;i<block_size/4;i++) buf[i] = random();
}

static void write_block(int fd, unsigned *buf, off_t i)
{
	do_seek(fd, i*(off_t)block_size);
	if (write(fd, buf, block_size) != block_size) {
		fprintf(stderr,"write failed on block %u\n", (unsigned)i);
		exit(1);
	}
}

static void check_block(int fd, unsigned *buf, off_t i)
{
	unsigned buf2[block_size/4];

	do_seek(fd, i*(off_t)block_size);
	if (read(fd, buf2, block_size) != block_size) {
		fprintf(stderr,"read failed on block %u\n", (unsigned)i);
		exit(1);
	}
	if (memcmp(buf, buf2, block_size) != 0) {
		int j, count=0, first = -1;
		for (j=0; j<block_size/4; j++) {
			if (buf[j] != buf2[j]) {
				count++;
				if (first == -1) first = j;
			}
		}
		printf("compare failed on block %u (%d words differ) 1st error at word %u\n", 
		       (unsigned)i, count, first);
		fd = open("buf1.dat", O_WRONLY|O_CREAT|O_TRUNC, 0644);
		write(fd, buf, block_size);
		close(fd);
		fd = open("buf2.dat", O_WRONLY|O_CREAT|O_TRUNC, 0644);
		write(fd, buf2, block_size);
		close(fd);
		printf("Wrote good data to buf1.dat\n");
		printf("Wrote bad data to buf2.dat\n");
		num_fails++;
	}
}


static void try_block(int fd, off_t i, unsigned char *bitmap)
{
	unsigned *buf;
	buf = (unsigned *)memalign(block_size, block_size);

	if (!bit_query(bitmap, i) || (random()%100 > read_prob)) {
		bit_set(bitmap, i);
		init_buf(buf, i);
		write_block(fd, buf, i);
		num_writes++;
	} else {
		init_buf(buf, i);
		check_block(fd, buf, i);
		num_reads++;
	}

	free(buf);
}


static void disk_test(char *dev)
{
	int fd;
	int count=0, count2;
	unsigned char *bitmap;
	off_t i, j;
	time_t t1, t2;
	unsigned blk;

	fd = open(dev, O_RDWR|O_SYNC|O_LARGEFILE);
	if (fd == -1) {
		perror(dev);
		return;
	}

	if (blocks == 0) {
		if (ioctl(fd, BLKGETSIZE, &blocks) != 0) {
			fprintf(stderr,"BLKGETSIZE failed\n");
			exit(1);
		}

		blocks /= (block_size/512);
	}

	bitmap = calloc((blocks+7)/8, 1);
	if (!bitmap) {
		fprintf(stderr,"malloc of bitmap failed\n");
		exit(1);
	}

	srandom(time(NULL));

	printf("starting disktest on %s with %u blocks of size %u\n", 
	       dev, blocks, block_size);
	printf("using read probability of %u%%\n", read_prob);

	t1 = time(NULL);
	count2 = 0;
	blk = 0;

	while (1) {
		i = ((unsigned)random()) % blocks;

		try_block(fd, blk % blocks, bitmap);
		try_block(fd, i, bitmap);

		blk++;
		count += 2;
		count2 += 2;

		t2 = time(NULL);

		if (t2 - t1 >= rep_time) {
			printf("%s %d blocks (%d reads, %d writes, %d fails, %.2fMB/sec)\n",
			       dev, count, num_reads, num_writes, num_fails, 
			       1.0e-6*(count2*block_size)/(t2-t1));
			srandom(random() | time(NULL));
			t1 = t2;
			count2=0;
		}
		fflush(stdout);
	}

	close(fd);
}


static void usage(void)
{
	fprintf(stderr,
"\n" \
"usage: disktest [options] <device>\n" \
"\n" \
"options: \n" \
"         -B <blocks>           set block count\n" \
"         -s <blk_size>         set block size\n" \
"         -R <value>            set read probability (1-100)\n" \
"         -t <value>            set report time in seconds\n" \
"\n");
	exit(1);
}


int main(int argc, char *argv[])
{
	char *dev;
	int opt;
	extern char *optarg;

	while ((opt = getopt(argc, argv, "B:s:R:t:h")) != EOF) {
		switch (opt) {
		case 'B':
			blocks = atoi(optarg);
			break;
		case 's':
			block_size = atoi(optarg);
			break;
		case 'R':
			read_prob = atoi(optarg);
			break;
		case 't':
			rep_time = atoi(optarg);
			break;
		default:
			usage();
			break;
		}
	}

	argv += optind;
	argc -= optind;

	if (argc < 1) usage();

	dev = argv[0];

	setlinebuf(stdout);

	disk_test(dev);

	return 0;
}

