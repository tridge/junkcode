/*
  A trivial ramdisk
  tridge@valinux.com, 2001
*/

#define MAJOR_NR   240

#define DEVICE_NAME "trd"
#define DEVICE_REQUEST do_trd_request
#define DEVICE_NR(device) (MINOR(device))

#include <linux/major.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/blk.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/bootmem.h>
#include <asm/setup.h>
#include <asm/uaccess.h>

#define TRD_BLOCK_SIZE 1024
#define TRD_CHUNK_SIZE (1024*TRD_BLOCK_SIZE)
#define TRD_SIZE (trd_size<<10)

static int trd_blocksizes[1] = {TRD_BLOCK_SIZE};
static void **trd_base;
static int trd_chunks;
static unsigned trd_size = 4096;

MODULE_PARM (trd_size, "1i");

static void do_trd_request(request_queue_t * q)
{
	u_long start, ofs, len, len1;
	void *addr;
	int chunk;

	while (1) {
		INIT_REQUEST;
	    
		start = CURRENT->sector << 9;
		len  = CURRENT->current_nr_sectors << 9;
		
		if ((start + len) > TRD_SIZE) {
			printk(KERN_ERR DEVICE_NAME ": bad access: block=%ld, count=%ld\n",
			       CURRENT->sector,
			       CURRENT->current_nr_sectors);
			end_request(0);
			continue;
		}
		
		if ((CURRENT->cmd != READ) && (CURRENT->cmd != WRITE)) {
			printk(KERN_ERR DEVICE_NAME ": bad command: %d\n", CURRENT->cmd);
			end_request(0);
			continue;
		}

		while (len) {
			chunk = start / TRD_CHUNK_SIZE;
			ofs = start % TRD_CHUNK_SIZE;
			addr = (void *) (((char *)(trd_base[chunk])) + ofs);

			len1 = len;
			if (ofs + len1 > TRD_CHUNK_SIZE) {
				len1 = TRD_CHUNK_SIZE - ofs;
			}

			if (CURRENT->cmd == READ) {
				memcpy(CURRENT->buffer, (char *)addr, len1);
			} else {
				memcpy((char *)addr, CURRENT->buffer, len1);
			}

			len -= len1;
			start += len1;
		}
	    
		end_request(1);
	}
}

static int trd_open(struct inode *inode, struct file *filp)
{
	if (trd_base == NULL) return -ENOMEM;

	printk(KERN_DEBUG DEVICE_NAME ": trd opened\n");

	MOD_INC_USE_COUNT;

	return 0;
}

static int trd_release(struct inode *inode, struct file *filp)
{
	MOD_DEC_USE_COUNT;
	printk(KERN_DEBUG DEVICE_NAME ": trd released\n");
	return 0;     
}

static int trd_ioctl(struct inode *inode, struct file *file,
		     unsigned int cmd, unsigned long arg)
{
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	if (!inode)
		return -EINVAL;

	switch (cmd) {
	case BLKGETSIZE:
		return put_user(TRD_SIZE >> 9, (long *) arg);
	}
	return -EINVAL;
}

static struct block_device_operations trd_fops =
{
	open:		trd_open,
	release:	trd_release,
	ioctl:		trd_ioctl,
};

static int trd_init(void)
{
	int i;

	trd_chunks = (TRD_SIZE + (TRD_CHUNK_SIZE-1)) / TRD_CHUNK_SIZE;
	trd_base = (void **)vmalloc(sizeof(void *)*trd_chunks);
	if (!trd_base) goto nomem;
	memset(trd_base, 0, sizeof(void *)*trd_chunks);

	for (i=0;i<trd_chunks-1;i++) {
		trd_base[i] = vmalloc(TRD_CHUNK_SIZE);
		if (!trd_base[i]) goto nomem;
	}
	trd_base[i] = vmalloc(TRD_SIZE - (i-1)*TRD_CHUNK_SIZE);
	if (!trd_base[i]) goto nomem;

	if (register_blkdev(MAJOR_NR, DEVICE_NAME, &trd_fops)) {
		printk(KERN_ERR DEVICE_NAME ": Unable to register_blkdev(%d)\n", MAJOR_NR);
		return -EBUSY;
	}

	blk_init_queue(BLK_DEFAULT_QUEUE(MAJOR_NR), DEVICE_REQUEST);
	blksize_size[MAJOR_NR] = trd_blocksizes;
	blk_size[MAJOR_NR] = &trd_size;

	printk(KERN_DEBUG DEVICE_NAME ": trd initialised size=%d\n", TRD_SIZE);

	return 0;

nomem:
	if (trd_base) {
		for (i=0;i<trd_chunks;i++) {
			if (trd_base[i]) vfree(trd_base[i]);
		}
		vfree(trd_base);
		trd_base = NULL;
	}	

	printk(KERN_ERR DEVICE_NAME ": Unable to allocate trd of size %d\n",
	       TRD_SIZE);
	return -ENOMEM;
}

static void __exit trd_cleanup(void)
{
	int i;

	unregister_blkdev(MAJOR_NR, DEVICE_NAME);

	for (i=0;i<trd_chunks;i++) {
		vfree(trd_base[i]);
	}
	vfree(trd_base);
	trd_base = NULL;
}

module_init(trd_init);
module_exit(trd_cleanup);
MODULE_DESCRIPTION("trivial ramdisk");
