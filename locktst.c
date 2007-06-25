/* simple program to test file locking */
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <errno.h>

#define True (1==1)
#define False (!True)

#define BOOL int
#define DEBUG(x,body) (printf body)
#define HAVE_FCNTL_LOCK 1

#ifndef strerror
#define strerror(i) sys_errlist[i]
#endif

/****************************************************************************
simple routines to do connection counting
****************************************************************************/
BOOL fcntl_lock(int fd,int op,int offset,int count,int type)
{
#if HAVE_FCNTL_LOCK
  struct flock lock;
  int ret;
  unsigned long mask = (1<<31);

  /* interpret negative counts as large numbers */
  if (count < 0)
    count &= ~mask;

  /* no negative offsets */
  offset &= ~mask;

  /* count + offset must be in range */
  while ((offset < 0 || (offset + count < 0)) && mask)
    {
      offset &= ~mask;
      mask = mask >> 1;
    }

  DEBUG(5,("fcntl_lock %d %d %d %d %d\n",fd,op,offset,count,type));

  lock.l_type = type;
  lock.l_whence = SEEK_SET;
  lock.l_start = offset;
  lock.l_len = count;
  lock.l_pid = 0;

  errno = 0;

  ret = fcntl(fd,op,&lock);

  if (errno != 0)
    DEBUG(3,("fcntl lock gave errno %d (%s)\n",errno,strerror(errno)));

  /* a lock query */
  if (op == F_GETLK)
    {
      if ((ret != -1) &&
	  (lock.l_type != F_UNLCK) && 
	  (lock.l_pid != 0) && 
	  (lock.l_pid != getpid()))
	{
	  DEBUG(3,("fd %d is locked by pid %d\n",fd,lock.l_pid));
	  return(True);
	}

      /* it must be not locked or locked by me */
      return(False);
    }

  /* a lock set or unset */
  if (ret == -1)
    {
      DEBUG(3,("lock failed at offset %d count %d op %d type %d (%s)\n",
	       offset,count,op,type,strerror(errno)));

      /* perhaps it doesn't support this sort of locking?? */
      if (errno == EINVAL)
	{
	  DEBUG(3,("locking not supported? returning True\n"));
	  return(True);
	}

      return(False);
    }

  /* everything went OK */
  return(True);
#else
  return(False);
#endif
}

void lock_test(char *fname)
{
  int fd = open(fname,O_RDWR | O_CREAT,0644);
  if (fd < 0) return;

  write(fd, "hello", 5);

  fcntl_lock(fd,F_SETLK,1,1,F_WRLCK);

  sleep(60);

  fcntl_lock(fd,F_GETLK,0,0,F_WRLCK);
  fcntl_lock(fd,F_SETLK,0,0,F_UNLCK);

  fcntl_lock(fd,F_SETLK,0,20,F_WRLCK);
  fcntl_lock(fd,F_GETLK,0,20,F_WRLCK);
  fcntl_lock(fd,F_SETLK,0,20,F_UNLCK);

  fcntl_lock(fd,F_SETLK,0x7FFFFF80,20,F_WRLCK);
  fcntl_lock(fd,F_GETLK,0x7FFFFF80,20,F_WRLCK);
  fcntl_lock(fd,F_SETLK,0x7FFFFF80,20,F_UNLCK);

  fcntl_lock(fd,F_SETLK,0xFFFFFF80,20,F_WRLCK);
  fcntl_lock(fd,F_GETLK,0xFFFFFF80,20,F_WRLCK);
  fcntl_lock(fd,F_SETLK,0xFFFFFF80,20,F_UNLCK);

  fcntl_lock(fd,F_SETLK,0xFFFFFFFF,20,F_WRLCK);
  fcntl_lock(fd,F_GETLK,0xFFFFFFFF,20,F_WRLCK);
  fcntl_lock(fd,F_SETLK,0xFFFFFFFF,20,F_UNLCK);

}


main(int argc,char *argv[])
{
	lock_test(argv[1]);
}
