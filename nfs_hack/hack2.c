#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/map.h>
#include <sys/debug.h>
#include <sys/modctl.h>
#include <sys/kmem.h>
#include <sys/cmn_err.h>
#include <sys/dmaga.h>
#include <sys/open.h>
#include <sys/stat.h>
#include <sys/cred.h>
#include <sys/mman.h>
#include <sys/vm.h>
#include <sys/poll.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/signal.h>
#include <sys/msgbuf.h>
#include <rpc/rpc.h>
#include <nfs/nfs.h>
#include <nfs/nfssys.h>
#include <sys/time.h>

#define RPC_OFFSET 0

#define DEBUG 0

#define PLINE cmn_err(CE_CONT,"%s(%d)\n", __FUNCTION__, __LINE__);

static unsigned long tick1, tick2;

static __inline__ unsigned long gettick(void)
{
	unsigned long ticks;

	__asm__ __volatile__("
                              rd %%tick, %0
                             "
			     : "=r" (ticks));

	return ticks;
}

static inline void timestart(void)
{
	tick1 = gettick();
}

/* return time difference in microseconds */
static inline long timediff(void)
{
	tick2 = gettick();
	return (tick2 - tick1) / 167;
}


static int (*orig_vop_lookup)(struct vnode *, char *, struct vnode **,
			      struct pathname *, int, struct vnode *, struct cred *);

static int my_vop_lookup(struct vnode *v, char *n, struct vnode **v2,
			 struct pathname *p, int i, struct vnode *v3, struct cred *c)
{
	cmn_err(CE_CONT,"vop_lookup [%s] [%s]\n", n, p);
	return orig_vop_lookup(v, n, v2, p, i, v3, c);
}


static int (*orig_vfs_root)(struct vfs *, struct vnode **);

static int my_vfs_root(struct vfs *v, struct vnode **vv)
{
	int ret;
	cmn_err(CE_CONT,"vfs_root\n");
	ret = orig_vfs_root(v, vv);
	orig_vop_lookup = (*vv)->v_op->vop_lookup;
	(*vv)->v_op->vop_lookup = my_vop_lookup;
	return ret;
}

static int vsw_index;

int
_init(void)
{
	int ret, i;
	PLINE;
	ret = Xinit();
	cmn_err(CE_CONT,"_init gave %d\n", ret);

	for (i=0;i<nfstype;i++) {
		cmn_err(CE_CONT,"filesystem [%s]\n", vfssw[i].vsw_name);
		if (strcmp("nfs", vfssw[i].vsw_name) == 0) {
			vsw_index = i;
		}
	}

	i = vsw_index;

	orig_vfs_root = vfssw[i].vsw_vfsops->vfs_root;
	vfssw[i].vsw_vfsops->vfs_root = my_vfs_root;
	
	return ret;
}

int
_info(struct modinfo *m)
{
	int ret;
	PLINE;
	ret = Xinfo(m);
	cmn_err(CE_CONT,"_info gave %d\n", ret);
	return ret;
}

int _fini()
{
	int ret;
	PLINE;
	ret = Xfini();
	cmn_err(CE_CONT,"_fini gave %d\n", ret);
	return 0;
}

bool_t Xdr_bytes(XDR *xdrs, char **sp, u_int *sizep,
		 const u_int maxsize)
{
	bool_t ret;

	timestart();
	ret = xdr_bytes(xdrs, sp, sizep, maxsize);

	cmn_err(CE_CONT,"hack2 xdr_bytes sizep=%d maxsize=%d -> %d (time=%d)\n", 
		*sizep, maxsize, ret, timediff());
	return ret;
}

bool_t Xdr_opaque(XDR *xdrs, caddr_t cp, const u_int cnt)
{
	bool_t ret;

	ret = xdr_opaque(xdrs, cp, cnt);

#if DEBUG
	cmn_err(CE_CONT,"xdr_opaque cnt=%d -> %d\n", cnt, ret);
#endif
	return ret;
}

int Xopyout(caddr_t driverbuf, caddr_t userbuf, size_t cn)
{	
	int ret;
	timestart();
	ret = copyout(driverbuf, userbuf, cn);
	cmn_err(CE_CONT,"copyout(%x, %x, %d) -> ret (%d us)\n", 
		driverbuf, userbuf, cn, ret, timediff());
	return ret;
}
