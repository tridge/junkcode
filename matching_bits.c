#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

typedef unsigned char uchar;

/****************************************************************************
return the number of bits that match between two 4 character buffers
  ***************************************************************************/
int matching_quad_bits(uchar *p1, uchar *p2)
{
	int i, j, ret = 0;
	for (i=0; i<4; i++) {
		if (p1[i] != p2[i]) break;
		ret += 8;
	}

	if (i==4) return ret;

	for (j=0; j<8; j++) {
		if ((p1[i] & (1<<(7-j))) != (p2[i] & (1<<(7-j)))) break;
		ret++;
	}	
	
	return ret;
}


int main(int argc, char *argv[])
{
	struct in_addr ip1, ip2;

	if (argc < 3) {
		printf("Usage: matching_bits IP1 IP2\n");
		exit(1);
	}

	inet_aton(argv[1], &ip1);
	inet_aton(argv[2], &ip2);

	printf("%d\n", matching_quad_bits((uchar *)&ip1.s_addr, (uchar *)&ip2.s_addr));
	return 0;
}
