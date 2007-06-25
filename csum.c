#define __KERNEL__

#include <linux/config.h>
#include <asm/segment.h>
#include <asm/system.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/config.h>

#include <linux/socket.h>
#include <linux/sockios.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include <linux/icmp.h>
#include <linux/udp.h>
#include <net/ip.h>
#include <net/protocol.h>
#include <net/route.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <net/icmp.h>
#include <linux/firewall.h>
#include <linux/ip_fw.h>
#include <net/checksum.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>
#include <net/checksum.h>

/*
 * computes a partial checksum, e.g. for TCP/UDP fragments
 */

unsigned int csum_partial(const unsigned char * buff, int len, unsigned int sum) {
	  /*
	   * Experiments with ethernet and slip connections show that buff
	   * is aligned on either a 2-byte or 4-byte boundary.  We get at
	   * least a 2x speedup on 486 and Pentium if it is 4-byte aligned.
	   * Fortunately, it is easy to convert 2-byte alignment to 4-byte
	   * alignment for the unrolled loop.
	   */
	__asm__("
	    testl $2, %%esi		# Check alignment.
	    jz 2f			# Jump if alignment is ok.
	    subl $2, %%ecx		# Alignment uses up two bytes.
	    jae 1f			# Jump if we had at least two bytes.
	    addl $2, %%ecx		# ecx was < 2.  Deal with it.
	    jmp 4f
1:	    movw (%%esi), %%bx
	    addl $2, %%esi
	    addw %%bx, %%ax
	    adcl $0, %%eax
2:
	    movl %%ecx, %%edx
	    shrl $5, %%ecx
	    jz 2f
	    testl %%esi, %%esi
1:	    movl (%%esi), %%ebx
	    adcl %%ebx, %%eax
	    movl 4(%%esi), %%ebx
	    adcl %%ebx, %%eax
	    movl 8(%%esi), %%ebx
	    adcl %%ebx, %%eax
	    movl 12(%%esi), %%ebx
	    adcl %%ebx, %%eax
	    movl 16(%%esi), %%ebx
	    adcl %%ebx, %%eax
	    movl 20(%%esi), %%ebx
	    adcl %%ebx, %%eax
	    movl 24(%%esi), %%ebx
	    adcl %%ebx, %%eax
	    movl 28(%%esi), %%ebx
	    adcl %%ebx, %%eax
	    lea 32(%%esi), %%esi
	    dec %%ecx
	    jne 1b
	    adcl $0, %%eax
2:	    movl %%edx, %%ecx
	    andl $0x1c, %%edx
	    je 4f
	    shrl $2, %%edx
	    testl %%esi, %%esi
3:	    adcl (%%esi), %%eax
	    lea 4(%%esi), %%esi
	    dec %%edx
	    jne 3b
	    adcl $0, %%eax
4:	    andl $3, %%ecx
	    jz 7f
	    cmpl $2, %%ecx
	    jb 5f
	    movw (%%esi),%%cx
	    leal 2(%%esi),%%esi
	    je 6f
	    shll $16,%%ecx
5:	    movb (%%esi),%%cl
6:	    addl %%ecx,%%eax
	    adcl $0, %%eax
7:	    "
	: "=a"(sum)
	: "0"(sum), "c"(len), "S"(buff)
	: "bx", "cx", "dx", "si");
	return(sum);
}

/* 32 bits version of the checksum routines written for the Alpha by Linus */
static unsigned short from32to16(unsigned long x)
{
	/* add up 16-bit and 17-bit words for 17+c bits */
	x = (x & 0xffff) + (x >> 16);
	/* add up 16-bit and 2-bit for 16+c bit */
	x = (x & 0xffff) + (x >> 16);
	/* add up carry.. */
	x = (x & 0xffff) + (x >> 16);
	return x;
}


main(int argc,char *argv[])
{
  unsigned long sum1,sum2,sum3,sum4,sum5;
  int i;
  __u32 saddr=0x32456787,daddr=0x89764512;
  int len=512,proto=7;
  char data1[512];
  int seed = time(0);

  if (argc > 1) seed = atoi(argv[1]);

  while (1) {
    srandom(seed);
    for (i=0; i<512 ; i++) {
      data1[i] = random();
    }

  sum1 = csum_tcpudp_magic(saddr,daddr,len,proto,csum_partial(data1,512,0));

  sum2 = csum_tcpudp_magic(saddr,daddr,len,proto,csum_partial(data1,256,0));
  
  data1[4]++;
  daddr ^= 0x765432FE;

  sum3 = csum_tcpudp_magic(saddr,daddr,len,proto,csum_partial(data1,256,0));
  
  sum4 = ~sum3 + (~sum1 - ~sum2);
  if (!(sum4>>16)) sum4++;
  sum4 = 0xFFFF & (~from32to16(sum4));

  sum5 = csum_tcpudp_magic(saddr,daddr,len,proto,csum_partial(data1,512,0));

  if (sum4 != sum5) {
    printf("Failed with seed=%d\n",seed);
    exit(1);
  }
  seed++;
  if (!(seed & 0xFFF))
    printf("seed=0x%X\n",seed);
  }

  printf("%X %X\n",~sum3,~sum3 + (~sum1 - ~sum2));
  printf("sum1=%X\n",(unsigned int)sum1 & 0xFFFF);
  printf("sum2=%X\n",(unsigned int)sum2 & 0xFFFF);
  printf("sum3=%X\n",(unsigned int)sum3 & 0xFFFF);
  printf("sum4=%X\n",(unsigned int)sum4 & 0xFFFF);
  printf("sum5=%X\n",(unsigned int)sum5 & 0xFFFF);
}
