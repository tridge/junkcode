#include <stdio.h>
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

typedef unsigned short uint16;
typedef unsigned int uint32;

#if (defined(__powerpc__) && defined(__GNUC__))
static __inline__ uint16_t ld_le16(const uint16_t *addr)
{
	uint16_t val;
	__asm__ ("lhbrx %0,0,%1" : "=r" (val) : "r" (addr), "m" (*addr));
	return val;
}

static __inline__ void st_le16(uint16_t *addr, const uint16_t val)
{
	__asm__ ("sthbrx %1,0,%2" : "=m" (*addr) : "r" (val), "r" (addr));
}

static __inline__ uint32_t ld_le32(const uint32_t *addr)
{
	uint32_t val;
	__asm__ ("lwbrx %0,0,%1" : "=r" (val) : "r" (addr), "m" (*addr));
	return val;
}

static __inline__ void st_le32(uint32_t *addr, const uint32_t val)
{
	__asm__ ("stwbrx %1,0,%2" : "=m" (*addr) : "r" (val), "r" (addr));
}
#define HAVE_ASM_BYTEORDER 1
#endif



#undef CAREFUL_ALIGNMENT

/* we know that the 386 can handle misalignment and has the "right" 
   byteorder */
#if defined(__i386__)
#define CAREFUL_ALIGNMENT 0
#endif

#ifndef CAREFUL_ALIGNMENT
#define CAREFUL_ALIGNMENT 1
#endif

#define CVAL(buf,pos) ((uint_t)(((const uint8_t *)(buf))[pos]))
#define CVAL_NC(buf,pos) (((uint8_t *)(buf))[pos]) /* Non-const version of CVAL */
#define PVAL(buf,pos) (CVAL(buf,pos))
#define SCVAL(buf,pos,val) (CVAL_NC(buf,pos) = (val))

#if HAVE_ASM_BYTEORDER

#define  _PTRPOS(buf,pos) (((const uint8_t *)buf)+(pos))
#define SVAL(buf,pos) ld_le16((const uint16_t *)_PTRPOS(buf,pos))
#define IVAL(buf,pos) ld_le32((const uint32_t *)_PTRPOS(buf,pos))
#define SSVAL(buf,pos,val) st_le16((uint16_t *)_PTRPOS(buf,pos), val)
#define SIVAL(buf,pos,val) st_le32((uint32_t *)_PTRPOS(buf,pos), val)
#define SVALS(buf,pos) ((int16_t)SVAL(buf,pos))
#define IVALS(buf,pos) ((int32_t)IVAL(buf,pos))
#define SSVALS(buf,pos,val) SSVAL((buf),(pos),((int16_t)(val)))
#define SIVALS(buf,pos,val) SIVAL((buf),(pos),((int32_t)(val)))

#elif CAREFUL_ALIGNMENT

#define SVAL(buf,pos) (PVAL(buf,pos)|PVAL(buf,(pos)+1)<<8)
#define IVAL(buf,pos) (SVAL(buf,pos)|SVAL(buf,(pos)+2)<<16)
#define SSVALX(buf,pos,val) (CVAL_NC(buf,pos)=(uint8_t)((val)&0xFF),CVAL_NC(buf,pos+1)=(uint8_t)((val)>>8))
#define SIVALX(buf,pos,val) (SSVALX(buf,pos,val&0xFFFF),SSVALX(buf,pos+2,val>>16))
#define SVALS(buf,pos) ((int16_t)SVAL(buf,pos))
#define IVALS(buf,pos) ((int32_t)IVAL(buf,pos))
#define SSVAL(buf,pos,val) SSVALX((buf),(pos),((uint16_t)(val)))
#define SIVAL(buf,pos,val) SIVALX((buf),(pos),((uint32_t)(val)))
#define SSVALS(buf,pos,val) SSVALX((buf),(pos),((int16_t)(val)))
#define SIVALS(buf,pos,val) SIVALX((buf),(pos),((int32_t)(val)))

#else /* CAREFUL_ALIGNMENT */

/* this handles things for architectures like the 386 that can handle
   alignment errors */
/*
   WARNING: This section is dependent on the length of int16_t and int32_t
   being correct 
*/

/* get single value from an SMB buffer */
#define SVAL(buf,pos) (*(const uint16_t *)((const char *)(buf) + (pos)))
#define SVAL_NC(buf,pos) (*(uint16_t *)((char *)(buf) + (pos))) /* Non const version of above. */
#define IVAL(buf,pos) (*(const uint32_t *)((const char *)(buf) + (pos)))
#define IVAL_NC(buf,pos) (*(uint32_t *)((char *)(buf) + (pos))) /* Non const version of above. */
#define SVALS(buf,pos) (*(const int16_t *)((const char *)(buf) + (pos)))
#define SVALS_NC(buf,pos) (*(int16_t *)((char *)(buf) + (pos))) /* Non const version of above. */
#define IVALS(buf,pos) (*(const int32_t *)((const char *)(buf) + (pos)))
#define IVALS_NC(buf,pos) (*(int32_t *)((char *)(buf) + (pos))) /* Non const version of above. */

/* store single value in an SMB buffer */
#define SSVAL(buf,pos,val) SVAL_NC(buf,pos)=((uint16_t)(val))
#define SIVAL(buf,pos,val) IVAL_NC(buf,pos)=((uint32_t)(val))
#define SSVALS(buf,pos,val) SVALS_NC(buf,pos)=((int16_t)(val))
#define SIVALS(buf,pos,val) IVALS_NC(buf,pos)=((int32_t)(val))

#endif /* CAREFUL_ALIGNMENT */


static void aligntest(char *buf, size_t size)
{
	int i;
	uint32 val=0;
	for (i=0;i<size-4;i++) {
		val += IVAL(buf, i);
	}
}

int main(void)
{
	char *buf = calloc(1, 5000);
	int count;

	start_timer();
	for (count=0;end_timer() < 5.0;count++) {
		aligntest1(buf, 5000);
	}
	printf("test1 %.2f loops/sec\n", count/end_timer());

	return 0;
}
