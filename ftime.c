#include <sys/types.h>
#include <utime.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

main()
{
  struct utimbuf times;
  int fd = open("xx",O_RDWR|O_CREAT|O_TRUNC,0666);

  write(fd,"hello",6);

  bzero(&times,sizeof(times));

  times.actime = 3600;
  times.modtime = 7200;

  utime("xx",&times);

  close(fd);
}
