/* use TCP acks to measure lag. Call this after sending critcal data
   and it will return the number of milliseconds it took for all the
   data to be acknowledged by the clients TCP stack 

   this is very CPU intensive, for a real application you would need
   to do this in a timer or at least yield between ioctl() calls

   -1 is returned on error
*/
int measure_lag(int fd)
{
	struct timeval tv1, tv2;
	int outq;

#ifndef TIOCOUTQ
#define TIOCOUTQ 0x5411
#endif

	if (gettimeofday(&tv1, NULL) != 0) {
		return -1;
	}

	while (ioctl(fd, TIOCOUTQ, &outq) == 0 && outq != 0) ;

	if (gettimeofday(&tv2, NULL) != 0) {
		return -1;
	}
	
	return (tv2.tv_sec - tv1.tv_sec) * 1000 +
		(tv2.tv_usec - tv1.tv_usec) / 1000;
}

