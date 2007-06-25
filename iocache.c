#define _XOPEN_SOURCE 500
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libaio.h>
#include <getopt.h>
#include <errno.h>

#include <sys/time.h>
#include <time.h>

static struct timeval tp1,tp2;

static void start_timer()
{
	gettimeofday(&tp1,NULL);
}

static double end_timer()
{
	gettimeofday(&tp2,NULL);
	return (tp2.tv_sec + (tp2.tv_usec*1.0e-6)) - 
		(tp1.tv_sec + (tp1.tv_usec*1.0e-6));
}


struct io_block {
	struct iocb *cb;
	unsigned char *buffer;
	int nread;
	int done;
	int blk;
};

struct io_ring {
	int fd;
	unsigned block_size;
	unsigned num_blocks;
	struct io_block *blocks;
	io_context_t io_ctx;
};

static int a_schedule(struct io_ring *ring, int blk)
{
	struct io_block *b = &ring->blocks[blk % ring->num_blocks];
	
	if (blk == b->blk) return 0;

	if (b->buffer == NULL) {
		b->buffer = malloc(ring->block_size);
		if (b->buffer == NULL) goto failed;
	}

	if (b->cb == NULL) {
		b->cb = malloc(sizeof(*b->cb));
		if (b->cb == NULL) goto failed;
	}

	if (b->blk != -1 && !b->done) {
		struct io_event res;
		int ret = io_cancel(ring->io_ctx, b->cb, &res);
		if (ret == EAGAIN) {
			goto failed;
		}
	}

	b->blk = blk;
	b->done = 0;
	memset(b->cb, 0, sizeof(*b->cb));
        io_prep_pread(b->cb, ring->fd, b->buffer, ring->block_size,
	              blk * (off_t)ring->block_size);
	if (io_submit(ring->io_ctx, 1, &b->cb) != 0) goto failed;
	b->cb->key = blk;
	return 0;

failed:
	free(b->buffer);
	free(b->cb);
	b->buffer = NULL;
	b->cb     = NULL;
	b->blk    = -1;
	return -1;
}

static int a_wait(struct io_ring *ring, int blk)
{
	struct io_block *b = &ring->blocks[blk % ring->num_blocks];
	struct io_event ev[10];

	if (b->blk != blk) return -1;

	while (!b->done) {
		int i, nr = io_getevents(ring->io_ctx, 1, 10, ev, NULL);
		for (i=0;i<nr;i++) {
			struct iocb *cb = ev[i].obj;
			struct io_block *b2 = &ring->blocks[cb->key % ring->num_blocks];
			b2->done = 1;
			b2->nread = ev[i].res;
		}
	}

	if (b->nread < 0) return -1;

	return 0;
}


static ssize_t a_read(struct io_ring *ring, void *buf, size_t count, off_t offset)
{
	int blk_start = offset / ring->block_size;
	int blk_end   = (offset+count-1) / ring->block_size;
	int blk_sched = blk_start + (ring->num_blocks/2);
	int i;
	ssize_t total=0;

	if (blk_sched < blk_end) blk_sched = blk_end;

	for (i=blk_start;i<=blk_sched;i++) {
		a_schedule(ring, i);
	}

	while (count) {
		int n = count;
		int blk_offset = offset % ring->block_size;
		int blk = offset / ring->block_size;
		struct io_block *b = &ring->blocks[blk % ring->num_blocks];
		ssize_t nread;
		if (n > ring->block_size - blk_offset) {
			n = ring->block_size - blk_offset;
		}
		if (a_wait(ring, blk) != 0) {
			nread = pread(ring->fd, buf, n, offset);
			printf("sync fallback\n");
		} else {
			if (n > b->nread) n = b->nread;
			memcpy(buf, b->buffer + blk_offset, n);
			nread = n;
		}
		if (nread <= 0) {
			if (total > 0) return total;
			return nread;
		}
		total += nread;
		buf = (void *)(nread + (char *)buf);
		count -= nread;
		offset += nread;
	}

	return total;
}

static struct io_ring *a_init(int fd, int block_size, int nblocks)
{
	struct io_ring *ring;
	int i;
	int ret;

	ring = malloc(sizeof(*ring));
	if (ring == NULL) return NULL;

	memset(ring, 0, sizeof(*ring));
	ret = io_queue_init(nblocks, &ring->io_ctx);
	if (ret != 0) {
		free(ring);
		return NULL;
	}
	ring->fd = fd;
	ring->num_blocks = nblocks;
	ring->block_size = block_size;
	ring->blocks = calloc(nblocks, sizeof(ring->blocks[0]));
	if (ring->blocks == NULL) {
		free(ring);
		return NULL;
	}

	for (i=0;i<nblocks;i++) {
		ring->blocks[i].blk = -1;
	}
	return ring;
}

static void a_close(struct io_ring *ring)
{
	int i;

	for (i=0;i<ring->num_blocks;i++) {
		struct io_block *b = &ring->blocks[i];
		if (b->blk != -1 && !b->done) {
			struct io_event res;
			int ret = io_cancel(ring->io_ctx, b->cb, &res);
			if (ret == EAGAIN) {
				a_wait(ring, b->blk);
			}
		}
		if (b->cb) free(b->cb);
		if (b->buffer) free(b->buffer);
	}
	free(ring);
}

static void async_test(const char *fname, int block_size, int nblocks, int rsize)
{
	int fd;
	struct io_ring *ring;
	off_t offset=0;
	char buf[rsize];
	int i=0;

	offset=0;
	start_timer();
	fd = open(fname, O_RDONLY);
	if (fd == -1) {
		perror(fname);
		exit(1);
	}

	ring = a_init(fd, block_size, nblocks);
	if (ring == NULL) {
		fprintf(stderr, "a_init faild\n");
		exit(1);
	}

	while (a_read(ring, buf, sizeof(buf), offset) > 0) {
		offset += sizeof(buf);
		if (++i % 5 == 0) {
			offset -= 2*sizeof(buf);
		}
	}

	printf("async: %.3f MByte/sec\n", 1.0e-6 * offset / end_timer());
	a_close(ring);
}

static void sync_test(const char *fname, int rsize)
{
	int fd;
	off_t offset=0;
	char buf[rsize];
	int i=0;

	offset=0;
	start_timer();
	fd = open(fname, O_RDONLY);
	if (fd == -1) {
		perror(fname);
		exit(1);
	}

	while (pread(fd, buf, sizeof(buf), offset) > 0) {
		offset += sizeof(buf);
		if (++i % 5 == 0) {
			offset -= 2*sizeof(buf);
		}
	}

	printf("sync : %.3f MByte/sec\n", 1.0e-6 * offset / end_timer());
	close(fd);
}

static void usage(void)
{
	printf("Usage: aiocache [options] <filename>\n");
	printf(
"Options:\n" \
" -b block_size\n" \
" -n nblocks\n" \
" -r readsize\n");
}

int main(int argc, const char *argv[])
{
	const char *fname;
	int opt;
	int block_size = 1024*1024;
	int nblocks = 100;
	int rsize = 32*1024;

	while ((opt = getopt(argc, argv, "b:n:r:")) != -1) {
		switch (opt) {
		case 'b':
			block_size = strtol(optarg, NULL, 0);
			break;
		case 'n':
			nblocks = strtol(optarg, NULL, 0);
			break;
		case 'r':
			rsize = strtol(optarg, NULL, 0);
			break;
		default:
			printf("Invalid option '%c'\n", opt);
			usage();
			exit(1);
		}
	}

	argv += optind;
	argc -= optind;

	if (argc < 1) {
		usage();
		exit(1);
	}

	fname = argv[0];

	printf("Testing with block_size=%d nblocks=%d rsize=%d\n",
	       block_size,nblocks,rsize);
	
	async_test(fname, block_size, nblocks, rsize);
	sync_test(fname, rsize);
	async_test(fname, block_size, nblocks, rsize);
	sync_test(fname, rsize);

	return 0;	
}
