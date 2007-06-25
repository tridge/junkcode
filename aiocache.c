#define _XOPEN_SOURCE 500
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <aio.h>
#include <getopt.h>

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


struct aio_block {
	struct aiocb *cb;
	unsigned char *buffer;
	int nread;
	int done;
	int blk;
};

struct aio_ring {
	int fd;
	unsigned block_size;
	unsigned num_blocks;
	struct aio_block *blocks;
};

static int a_schedule(struct aio_ring *ring, int blk)
{
	struct aio_block *b = &ring->blocks[blk % ring->num_blocks];
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
		int ret = aio_cancel(ring->fd, b->cb);
		if (ret == AIO_NOTCANCELED) {
			aio_suspend(&b->cb, 1, NULL);
		}
		aio_error(b->cb);
		aio_return(b->cb);
	}

	b->blk = blk;
	memset(b->cb, 0, sizeof(*b->cb));
	b->cb->aio_fildes = ring->fd;
	b->cb->aio_buf     = b->buffer;
	b->cb->aio_nbytes  = ring->block_size;
	b->cb->aio_offset  = blk * (off_t)ring->block_size;
	if (aio_read(b->cb) != 0) goto failed;

	b->done = 0;
	return 0;

failed:
	free(b->buffer);
	free(b->cb);
	b->buffer = NULL;
	b->cb     = NULL;
	b->blk    = -1;
	return -1;
}

static int a_wait(struct aio_ring *ring, int blk)
{
	struct aio_block *b = &ring->blocks[blk % ring->num_blocks];

	if (b->blk != blk) return -1;

	if (b->done) return 0;

	if (aio_suspend(&b->cb, 1, NULL) != 0 ||
	    aio_error(b->cb) != 0) {
		free(b->buffer);
		free(b->cb);
		b->buffer = NULL;
		b->cb     = NULL;
		b->blk    = -1;
		return -1;
	}

	b->done = 1;
	b->nread = aio_return(b->cb);
	if (b->nread < 0) return -1;

	return 0;
}


static ssize_t a_read(struct aio_ring *ring, void *buf, size_t count, off_t offset)
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
		struct aio_block *b = &ring->blocks[blk % ring->num_blocks];
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

static struct aio_ring *a_init(int fd, int block_size, int nblocks)
{
	struct aio_ring *ring;
	int i;

	ring = malloc(sizeof(*ring));
	if (ring == NULL) return NULL;

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

static void a_close(struct aio_ring *ring)
{
	int i;

	for (i=0;i<ring->num_blocks;i++) {
		struct aio_block *b = &ring->blocks[i];
		if (b->blk != -1 && !b->done) {
			int ret = aio_cancel(ring->fd, b->cb);
			if (ret == AIO_NOTCANCELED) {
				aio_suspend(&b->cb, 1, NULL);
			}
			aio_error(b->cb);
			aio_return(b->cb);
		}
		if (b->cb) free(b->cb);
		if (b->buffer) free(b->buffer);
	}
	free(ring);
}

static void async_test(const char *fname, int block_size, int nblocks, int rsize)
{
	int fd;
	struct aio_ring *ring;
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
