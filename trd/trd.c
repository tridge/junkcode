/*
  A trivial ramdisk
  tridge@valinux.com, 2001
*/

#define MAJOR_NR   240
#define MAX_DEVS 32

#define DEVICE_NAME "trd"
#define DEVICE_NR(device) (MINOR(device))

#include <linux/major.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/blk.h>
#include <linux/init.h>
#include <linux/module.h>
#include <asm/setup.h>
#include <asm/uaccess.h>

#define TRD_BLOCK_SIZE 1024
#define TRD_SIZE (trd_size<<10)

static int trd_blocksizes[MAX_DEVS];
static int trd_blk_sizes[MAX_DEVS];
static void **trd_base[MAX_DEVS];
static int trd_pages;
static unsigned trd_size = 4096;

MODULE_PARM (trd_size, "1i");

static int trd_make_request(request_queue_t *q, int rw, struct buffer_head *bh)
{
	u_long start, len;
	char *b_addr;
	void *addr;
	u_long minor, page, ofs, len1;

	if (!trd_base[minor]) {
		int ret;
		printk(KERN_ERR DEVICE_NAME ": access to unopened device minor=%ld\n", minor);
		ret = trd_allocate(minor);
		if (ret != 0) return ret;
	}

	start = bh->b_rsector << 9;
	minor = MINOR(bh->b_rdev);
	len = bh->b_size;

	if (minor >= MAX_DEVS || !trd_base[minor]) {
		printk(KERN_ERR DEVICE_NAME ": bad minor %ld\n", minor);
		goto io_error;
	}

	if (start + bh->b_size > TRD_SIZE) {
		printk(KERN_ERR DEVICE_NAME ": bad access: block=%ld, count=%d\n",
			bh->b_rsector, bh->b_size);
		goto io_error;
	}

	if (rw == READA) rw = READ;

	if ((rw != READ) && (rw != WRITE)) {
		printk(KERN_ERR DEVICE_NAME ": bad command: %d\n", rw);
		goto io_error;
	}

	b_addr = bh_kmap(bh);

	while (len) {
		page = start >>  PAGE_SHIFT;
		ofs = start & (PAGE_SIZE-1);
		addr = (void *) (((char *)(trd_base[minor][page])) + ofs);
		
		len1 = len;
		if (ofs + len1 > PAGE_SIZE) {
			len1 = PAGE_SIZE - ofs;
		}
		
		if (rw == READ) {
			memcpy(b_addr, (char *)addr, len1);
		} else {
			memcpy((char *)addr, b_addr, len1);
		}
		
		len -= len1;
		start += len1;
		b_addr += len1;
	}

	bh_kunmap(bh);
	bh->b_end_io(bh,1);
	return 0;

io_error:
	bh->b_end_io(bh,0);
	return 0;
}

static int trd_allocate(int minor)
{
	int i;

	/* it might be already allocated */
	if (trd_base[minor]) return 0;

	trd_base[minor] = (void **)vmalloc(sizeof(void *)*trd_pages);
	if (!trd_base[minor]) goto nomem;
	memset(trd_base[minor], 0, sizeof(void *)*trd_pages);

	for (i=0;i<trd_pages;i++) {
		trd_base[minor][i] = (void *)__get_free_page(GFP_USER);
		if (!trd_base[minor][i]) goto nomem;
		/* we have to zero it to ensure private data doesn't leak */
		memset(trd_base[minor][i], 0, PAGE_SIZE);
	}

	printk(KERN_INFO DEVICE_NAME ": Allocated %dk to minor %d\n",
	       TRD_SIZE>>10, minor);

	return 0;

nomem:
	if (trd_base[minor]) {
		for (i=0;i<trd_pages;i++) {
			if (trd_base[minor][i]) 
				free_page((unsigned long)trd_base[minor][i]);
		}
		vfree(trd_base[minor]);
		trd_base[minor] = NULL;
	}	

	printk(KERN_ERR DEVICE_NAME ": Unable to allocate trd of size %d\n",
	       TRD_SIZE);
	return -ENOMEM;
}


static int trd_open(struct inode *inode, struct file *filp)
{
	int ret;
	int minor = MINOR(inode->i_rdev);

	if (minor >= MAX_DEVS) return -ENODEV;

	ret = trd_allocate(minor);
	if (ret != 0) return ret;

	MOD_INC_USE_COUNT;

	return 0;
}

static int trd_release(struct inode *inode, struct file *filp)
{
	MOD_DEC_USE_COUNT;
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
	default:
		printk(KERN_ERR DEVICE_NAME ": unknown ioctl 0x%x\n", cmd);
		break;
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

	trd_pages = (TRD_SIZE + (PAGE_SIZE-1)) >> PAGE_SHIFT;

	if (register_blkdev(MAJOR_NR, DEVICE_NAME, &trd_fops)) {
		printk(KERN_ERR DEVICE_NAME ": Unable to register_blkdev(%d)\n", MAJOR_NR);
		return -EBUSY;
	}

	blk_queue_make_request(BLK_DEFAULT_QUEUE(MAJOR_NR), trd_make_request);
	for (i=0;i<MAX_DEVS;i++) {
		trd_blocksizes[i] = TRD_BLOCK_SIZE;
		trd_blk_sizes[i] = trd_size;
	}

	blksize_size[MAJOR_NR] = trd_blocksizes;
	blk_size[MAJOR_NR] = trd_blk_sizes;

	printk(KERN_DEBUG DEVICE_NAME ": trd initialised size=%dk\n", 
	       trd_size);

	return 0;
}

static void __exit trd_cleanup(void)
{
	int minor, i;

	unregister_blkdev(MAJOR_NR, DEVICE_NAME);

	for (minor=0;minor<MAX_DEVS;minor++) {
		if (!trd_base[minor]) continue;
		for (i=0;i<trd_pages;i++) {
			free_page((unsigned long)trd_base[minor][i]);
		}
		vfree(trd_base[minor]);
		trd_base[minor] = NULL;
	}
	printk(KERN_DEBUG DEVICE_NAME ": trd released\n");
}

module_init(trd_init);
module_exit(trd_cleanup);
MODULE_DESCRIPTION("trivial ramdisk");
