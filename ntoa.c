#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>


main()
{
	struct in_addr ip;
	
	ip.s_addr = 0x12345678;

	if (strcmp(inet_ntoa(ip),"18.52.86.120") != 0 &&
	    strcmp(inet_ntoa(ip),"120.86.52.18") != 0) {
		fprintf(stderr,"broken inet_ntoa\n");
		exit(0);
	} 

	/* not a broken inet_ntoa */
	exit(1);
}
