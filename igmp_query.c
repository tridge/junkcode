/* simple igmp query tool

   Andrew Tridgell 2004
*/

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

#define DEBUG 0

#define MAX_PACKET_LENGTH 1600 
#define HISTORY_LENGTH 20

typedef unsigned char byte_t;

static void s_8(byte_t *pkt, int ofs, unsigned v)
{
	pkt[ofs] = v;
}

static void s_16(byte_t *pkt, int ofs, unsigned v)
{
	((unsigned short *)(pkt + ofs))[0] = htons(v);
}

static void s_32(byte_t *pkt, int ofs, unsigned v)
{
	((unsigned *)(pkt + ofs))[0] = htonl(v);
}


static unsigned ip_sum(byte_t *data, int len)
{
	int i;
	unsigned sum = 0;
	for (i=0;i<len;i+=2) {
		sum += ((unsigned short *)(data + i))[0];
		sum &= 0xFFFF;
	}
	return htons(~sum);
}


static int iface_index(const char *ifname)
{
	int fd;
	struct ifreq ifr;

	if (isdigit(ifname[0])) {
		return atoi(ifname);
	}

	fd = socket(PF_PACKET, SOCK_DGRAM, ETH_P_IP);
	if (fd == -1) {
		perror("socket");
		return -1;
	}

	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	if (ioctl(fd, SIOCGIFINDEX, &ifr) != 0) {
		perror("SIOCGIFINDEX");
		return -1;
	}

	close(fd);

	return ifr.ifr_ifindex;
}


int main(int argc, char *argv[])
{
	int fd;
	byte_t pkt[32];
	int ttl;
	in_addr_t src_ip, dst_ip, group_ip;
	struct sockaddr_ll ll;
	int ifindex;

	if (argc < 3) {
		printf("Usage igmp_query <ttl> <iface> <src>\n");
		exit(1);
	}


	fd = socket(PF_PACKET, SOCK_DGRAM, ETH_P_IP);
	if (fd == -1) {
		perror("eth_open");
		exit(1);
	}

	ttl = atoi(argv[1]);

	ifindex = iface_index(argv[2]);
	if (ifindex == -1) {
		printf("Bad interface name '%s'\n", argv[2]);
		exit(1);
	}

	src_ip   = inet_addr(argv[3]);
	dst_ip   = inet_addr("224.0.0.1");

	group_ip = dst_ip;

	memset(pkt, 0, sizeof(pkt));
	
	s_8 (pkt,  0, 0x46);
	s_16(pkt,  2, sizeof(pkt));
	s_8 (pkt,  8, ttl); 
	s_8 (pkt,  9, 2);
	s_32(pkt, 12, htonl(src_ip));
	s_32(pkt, 16, htonl(dst_ip));
	s_32(pkt, 20, 0x94040000); /* router alert */
	
	s_8 (pkt, 24, 0x11); /* membership report */
	s_8 (pkt, 25, 0x64); /* v2 */
	s_32(pkt, 28, 0);
	
	s_16(pkt, 26, ip_sum(pkt+24, 8)); /* igmp csum */
	
	s_16(pkt, 10, ip_sum(pkt, 32)-0x100); /* ip csum */
	
	ll.sll_family = AF_PACKET;
	ll.sll_protocol = htons(ETH_P_IP);
	ll.sll_ifindex = ifindex;
	ll.sll_hatype  = htons(ARPHRD_ETHER);
	ll.sll_pkttype = PACKET_MULTICAST;
	ll.sll_halen = 6;
	ll.sll_addr[0] = 1;
	ll.sll_addr[1] = 0;
	s_32(ll.sll_addr, 2, (0x5e<<24) | (ntohl(group_ip) & 0x7FFFFF));
	
	sendto(fd, pkt, sizeof(pkt), 0, (struct sockaddr *)&ll, sizeof(ll));
	
	return 0;
}
