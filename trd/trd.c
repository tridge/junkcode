/*
  A trivial ramdisk
  tridge@valinux.com, 2001
*/

#define MAJOR_NR   240

#define MAX_DEVS 32

#define DEVICE_NAME "trd"
#define DEVICE_REQUEST do_trd_request
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

static void do_trd_request(request_queue_t * q)
{
	u_long start, ofs, len, len1;
	void *addr;
	int page, minor;

	while (1) {
		INIT_REQUEST;

		minor = MINOR(CURRENT->rq_dev);
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
			page = start / PAGE_SIZE;
			ofs = start % PAGE_SIZE;
			addr = (void *) (((char *)(trd_base[minor][page])) + ofs);

			len1 = len;
			if (ofs + len1 > PAGE_SIZE) {
				len1 = PAGE_SIZE - ofs;
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

static int trd_allocate(int minor)
{
	int i;

	/* it might be already allocated */
	if (trd_base[minor]) return 0;

	trd_blocksizes[minor] = TRD_BLOCK_SIZE;
	trd_blk_sizes[minor] = trd_size;

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
	trd_pages = (TRD_SIZE + (PAGE_SIZE-1)) / PAGE_SIZE;

	if (register_blkdev(MAJOR_NR, DEVICE_NAME, &trd_fops)) {
		printk(KERN_ERR DEVICE_NAME ": Unable to register_blkdev(%d)\n", MAJOR_NR);
		return -EBUSY;
	}

	blk_init_queue(BLK_DEFAULT_QUEUE(MAJOR_NR), DEVICE_REQUEST);
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
