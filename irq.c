#define __USE_BSD_SIGNAL 1

#define IRQ 4

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>


#define IRQ_SET 1
#define IRQ_RELEASE 2
#define IRQ_INFO 3

struct irq_struct
{
  int irq;
  int process_id;
  int signal_num;
  struct file *file;
  unsigned char *irq_buffer;
  int buf_start;
  int buf_count;
  int buf_size;
}
 setting;


int fioctl(FILE *f,int cmd,void *arg)
{
return(ioctl(fileno(f),cmd,arg));
}


void handler(int irq)
{
printf("got signal %d\n",irq);
}


main()
{
int fd = open("/dev/irq",O_RDWR);
char buf[10];
int i;
char c = IRQ;
for (i=0;i<10;i++)
  buf[i] = IRQ;

setting.irq = IRQ;
setting.process_id = getpid();
setting.signal_num = SIGSEGV;
setting.buf_size = 100;

signal(SIGSEGV,handler);

printf("ioctl gave %d\n",ioctl(fd,IRQ_SET,&setting));

printf("irq %d\npid %d\nsig %d\ncount %d\nsize %d\n",
       setting.irq,
       setting.process_id,
       setting.signal_num,
       setting.buf_count,
       setting.buf_size);

c = IRQ;
printf("write gave %d\n",write(fd,buf,10));

printf("info gave %d\n",ioctl(fd,IRQ_INFO,&setting));
printf("irq %d\npid %d\nsig %d\ncount %d\nsize %d\n",
       setting.irq,
       setting.process_id,
       setting.signal_num,
       setting.buf_count,
       setting.buf_size);

printf("read gave %d\n",read(fd,&c,1));
printf("c is %d\n",(int)c);
printf("info gave %d\n",ioctl(fd,IRQ_INFO,&setting));
printf("irq %d\npid %d\nsig %d\ncount %d\nsize %d\n",
       setting.irq,
       setting.process_id,
       setting.signal_num,
       setting.buf_count,
       setting.buf_size);
while (1);
}




