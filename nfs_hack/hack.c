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


typedef int (*dispatch_f)(struct svc_req *, SVCXPRT *);

dispatch_f real_dispatch_200003;
dispatch_f real_dispatch_200227;

#define DISPATCH_CAST (void (*)())

static struct svc_req *current_request;

static void tick_calibrate(void)
{
	static int done;
	unsigned long lbolt1, lbolt2;

	if (done) return;
	done = 1;

	drv_getparm(LBOLT, &lbolt1);
	lbolt2 = lbolt1;
	while (lbolt2 == lbolt1) {
		drv_getparm(LBOLT, &lbolt2);
	}
	lbolt1 = lbolt2;
	timestart();
	while (lbolt2 == lbolt1) {
		drv_getparm(LBOLT, &lbolt2);
	}
	cmn_err(CE_CONT,"One tick = %d microseconds\n", timediff());
}

static void ramp(char *p, int len)
{
	int i;
	for (i=0;i<len;i++) {
		p[i] = 'A'+(i%27);
		if (p[i] == 'A'+26) p[i] = '\n';
	}
}

static void interpret_reply(struct svc_req *request, const caddr_t out)
{
	READ3res *res;

	switch (request->rq_proc) {
	case RFS_READ:
	    res = (READ3res *)out;
	    cmn_err(CE_CONT,"RFS_READ status=%d\n", res->status);
	    if (res->status == 0) {
		    cmn_err(CE_CONT,"size=%d eof=%d count=%d data_len=%d data=%x\n",
			    res->resok.size, res->resok.eof, 
			    res->resok.count,
			    res->resok.data.data_len,
			    res->resok.data.data_val);
	    }
	    break;
	default:
		break;
	}
}

static int dispatch_200003(struct svc_req *request, SVCXPRT *xprt)
{
#if DEBUG
  cmn_err(CE_CONT,"dispatch_200003(proc=%d)\n", request->rq_proc);
#endif
  current_request = request;
  timestart();
  return real_dispatch_200003(request, xprt);
}

static int dispatch_200227(struct svc_req *request, SVCXPRT *xprt)
{
#if DEBUG
  cmn_err(CE_CONT,"dispatch_200227(proc=%d)\n", request->rq_proc);
#endif
  current_request = request;
  timestart();
  return real_dispatch_200227(request, xprt);
}


int Xvc_register(SVCXPRT *xprt, u_long prognum, u_long versnum,
		 void (*dispatch)(), u_long protocol)
{
	int ret;
#if DEBUG
	cmn_err(CE_CONT,"svc_register(prognum=%d versnum=%d protocol=%d dispatch=%x)\n",
		prognum,versnum,protocol, (unsigned)dispatch);
#endif

	svc_unregister(prognum, versnum);

	/* we add an offset so that we can co-exist with a real NFS server */
	prognum += RPC_OFFSET;

	if (prognum == 100003 + RPC_OFFSET) {
		real_dispatch_200003 = (dispatch_f)dispatch;
		ret = svc_register(xprt, prognum,versnum,
				   DISPATCH_CAST dispatch_200003,protocol);
#if DEBUG
		cmn_err(CE_CONT,"register gave %d\n", ret);
#endif
		return ret;
	} else if (prognum == 100227 + RPC_OFFSET) {
		real_dispatch_200227 = (dispatch_f)dispatch;
		ret = svc_register(xprt, prognum,versnum,
				    DISPATCH_CAST dispatch_200227,protocol);
#if DEBUG
		cmn_err(CE_CONT,"register gave %d\n", ret);
#endif
		return ret;
	} 

	cmn_err(CE_CONT,"svc_register - unknown prognum %d\n", prognum);
	ret = svc_register(xprt, prognum,versnum,dispatch,protocol);
	return ret;
}

bool_t Xvc_sendreply(SVCXPRT *xprt,
		     const xdrproc_t outproc, const caddr_t out)
{
  int ret;
#if DEBUG
  cmn_err(CE_CONT,"svc_sendreply timediff=%d\n", timediff());
  interpret_reply(current_request, out);
  cmn_err(CE_CONT,"xp_ops.xp_reply=0x%x xp_msg_size=%d\n", 
	  (unsigned long)xprt->xp_ops->xp_reply, xprt->xp_msg_size);

  cmn_err(CE_CONT,"xp_reply=%x\n", (u_long)xprt->xp_ops->xp_reply);
#endif

  xprt->xp_msg_size = 16240;

  /* xp_reply is svc_cots_ksend */
  ret = svc_sendreply(xprt, outproc, out);

  return ret;
}

int Xvc_tli_kcreate(struct file *f, u_int i, SVCXPRT **svc)
{
#if DEBUG
	PLINE;
#endif
	tick_calibrate();
	return svc_tli_kcreate(f, i, svc);
}

int nfs_svc(struct nfs_svc_args *a)
{
#if DEBUG
	cmn_err(CE_CONT,"nfs_svc: fd=%d\n", a->fd);
#endif
	return Xfs_svc(a);
}

int
_init(void)
{
	PLINE;

#if 0
	*(int *)0x52878c34 = 128*1024;
	*(int *)0x52878140 = 128*1024;
#endif

	cmn_err(CE_CONT,"nfs3tsize=%d (%d)\n",
		nfs3tsize());
	return Xinit();
}

int
_info(struct modinfo *m)
{
	PLINE;
	return Xinfo(m);
}

int _fini()
{
	int ret;
	PLINE;
	ret = Xfini();
	cmn_err(CE_CONT,"_fini gave %d\n", ret);
	return 0;
}
