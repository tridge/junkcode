/*
  A dummy disk device. reads and write always succeed, but don't 
  actually do anything

  tridge@valinux.com, 2001
*/

#define MAJOR_NR   241

#define DEVICE_NAME "ddisk"
#define DEVICE_NR(device) (MINOR(device))

#include <linux/major.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/blk.h>
#include <linux/init.h>
#include <linux/module.h>
#include <asm/setup.h>
#include <asm/uaccess.h>

#define DUMMY_BLOCK_SIZE 1024

static int dummy_blocksizes[MAX_DEVS];
static int dummy_blk_sizes[MAX_DEVS];
static unsigned dummy_size = 4096;

MODULE_PARM (dummy_size, "1i");

static int dummy_make_request(request_queue_t *q, int rw, struct buffer_head *bh)
{
	u_long start;

	start = bh->b_rsector >> 1;

	if (start + (bh->b_size>>10) > dummy_size) {
		printk(KERN_ERR DEVICE_NAME ": bad access: block=%ld, count=%d\n",
			bh->b_rsector, bh->b_size);
		goto io_error;
	}

	bh->b_end_io(bh,1);
	return 0;

io_error:
	buffer_IO_error(bh);
	return 0;
}

static int dummy_open(struct inode *inode, struct file *filp)
{
	MOD_INC_USE_COUNT;
	return 0;
}

static int dummy_release(struct inode *inode, struct file *filp)
{
	MOD_DEC_USE_COUNT;
	return 0;     
}

static int dummy_ioctl(struct inode *inode, struct file *file,
		     unsigned int cmd, unsigned long arg)
{
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	if (!inode)
		return -EINVAL;

	switch (cmd) {
	case BLKGETSIZE:
		return put_user(dummy_size << 1, (long *) arg);
	default:
		printk(KERN_ERR DEVICE_NAME ": unknown ioctl 0x%x\n", cmd);
		break;
	}
	return -EINVAL;
}

static struct block_device_operations dummy_fops =
{
	open:		dummy_open,
	release:	dummy_release,
	ioctl:		dummy_ioctl,
};


static int dummy_init(void)
{
	int i;

	if (register_blkdev(MAJOR_NR, DEVICE_NAME, &dummy_fops)) {
		printk(KERN_ERR DEVICE_NAME ": Unable to register_blkdev(%d)\n", MAJOR_NR);
		return -EBUSY;
	}

	blk_queue_make_request(BLK_DEFAULT_QUEUE(MAJOR_NR), dummy_make_request);
	for (i=0;i<MAX_DEVS;i++) {
		dummy_blocksizes[i] = DUMMY_BLOCK_SIZE;
		dummy_blk_sizes[i] = dummy_size;
	}

	blksize_size[MAJOR_NR] = dummy_blocksizes;
	blk_size[MAJOR_NR] = dummy_blk_sizes;

	printk(KERN_DEBUG DEVICE_NAME ": dummy initialised size=%dk\n", 
	       dummy_size);

	return 0;
}

static void __exit dummy_cleanup(void)
{
	unregister_blkdev(MAJOR_NR, DEVICE_NAME);
	printk(KERN_DEBUG DEVICE_NAME ": dummy released\n");
}

module_init(dummy_init);
module_exit(dummy_cleanup);
MODULE_DESCRIPTION("trivial ramdisk");
