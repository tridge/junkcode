/* simple appletalk bridge for rodney

   Andrew Tridgell 1997
*/

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <endian.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>

#define DEBUG 0

#define MAX_PACKET_LENGTH 1600 
#define HISTORY_LENGTH 20

/* we should use /etc/ethers but I'm too lazy */
#if 0
#define HUB    "00:00:0C:13:6F:18"
#else
#define HUB    "00:00:C0:81:93:8E"
#endif
#define ETH0   "00:00:C0:1B:A2:B8"
#define ETH1   "00:00:C0:6A:71:99"

static int eth_fd0 = -1;
static int eth_fd1 = -1;

static char *if_eth0 = "eth0";	/* name of ethernet interface */
static char *if_eth1 = "eth1";	/* name of ethernet interface */

static char data[MAX_PACKET_LENGTH]; 

static unsigned char hub_addr[ETH_ALEN];
static unsigned char eth0_addr[ETH_ALEN];
static unsigned char eth1_addr[ETH_ALEN];

static unsigned history[HISTORY_LENGTH];
static int history_pointer;

static unsigned eth0_counter, eth1_counter, dup_counter, ip_counter;

/* parse an ethernet address from string format to a byte array */
static void parse_ether(unsigned char *addr, char *addr_str)
{
	unsigned x[6];
	if (sscanf(addr_str, "%x:%x:%x:%x:%x:%x",
		   &x[0], &x[1], &x[2], 
		   &x[3], &x[4], &x[5]) != 6) {
		fprintf(stderr,"couldn't parse %s\n", addr_str);
	} 
	addr[0] = x[0]; addr[1] = x[1]; addr[2] = x[2];
	addr[3] = x[3];	addr[4] = x[4];	addr[5] = x[5];
}

/* pretty-print an ethernet address */
static char *ether_str(unsigned char *addr)
{
	static char s[2][30];
	static int i;
	char *ret;

	ret = s[i];

	sprintf(ret,"%02x:%02x:%02x:%02x:%02x:%02x",
		addr[0], addr[1], addr[2], 
		addr[3], addr[4], addr[5]);

	i = (i+1)%2;

	return ret;
}


/* fetch the protocol field in a ethernet or 802 packet */
static unsigned get_protocol(char *data)
{
	unsigned short proto;

	/* its either at offset 12 or 20 */
	proto = *(unsigned short *)(data + 12);
	proto = ntohs(proto);

	if (proto < 1500) {
		/* it must be an 802 packet */
		proto = *(unsigned short *)(data + 20);
		proto = ntohs(proto);		
	}
	
	return proto;
}

/* a really simple checksum routine, probably quite hopeless */
static unsigned csum(char *data,int len)
{
	unsigned sum=0;
	int i;
	for (i=0;i<len;i++)
		sum ^= data[i] << ((i*15) % 28);
	return sum;
}


/* close the ethernet fd and set it back to non-promiscuous */
static void eth_close(int fd, char *interface)
{
	struct ifreq ifr;

	strcpy(ifr.ifr_name, interface);
	if(ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
		perror("SIOCGIFFLAGS");
		return;
	}

	ifr.ifr_flags &= ~IFF_PROMISC;
	if(ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
		perror("SIOCSIFFLAGS");
		return;
	}

	close(fd);
}



/* open an ethernet socket for the requested protocol and
   return a file descriptor that can be used for reading/writing packets
   to that interface
 
   pstr specifies what packet types to accept
 */
static int eth_open(char *interface, char *pstr)
{
	int fd;
	int protocol = -1;
	struct ifreq ifr;

	if (!strcmp(pstr,"802.2")) protocol=ETH_P_802_2;
	if (!strcmp(pstr,"802.3")) protocol=ETH_P_802_3;
	if (!strcmp(pstr,"etherII")) protocol=ETH_P_IPX;
	if (!strcmp(pstr,"echo")) protocol=ETH_P_ECHO;
	if (!strcmp(pstr,"x25")) protocol=ETH_P_X25;
	if (!strcmp(pstr,"arp")) protocol=ETH_P_ARP;
	if (!strcmp(pstr,"atalk")) protocol=ETH_P_ATALK;
	if (!strcmp(pstr,"aarp")) protocol=ETH_P_AARP;
	if (!strcmp(pstr,"all")) protocol=ETH_P_ALL;

	if (protocol == -1) {
		printf("Unknown protocol [%s]\n",pstr);
		return -1;
	}

	if ((fd = socket(AF_INET, SOCK_PACKET, htons(protocol)))<0) {
		perror("Error opening local ethernet socket\n");
		return -1;
	}

	strcpy(ifr.ifr_name, interface);
	if(ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
		perror("SIOCGIFFLAGS");
		return -1;
	}

	ifr.ifr_flags |= IFF_PROMISC;
	if(ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
		perror("SIOCSIFFLAGS");
		return -1;
	}

	return fd;
}


static int eth_write(int fd, char *data, int len,char *iface)
{
	struct sockaddr dst;

	dst.sa_family = AF_INET;
	strcpy(dst.sa_data,iface);

	return sendto(fd,data,len,0,&dst,sizeof(dst));
}

static int eth_read(int fd, char *buf)
{
	struct sockaddr_in from;
	size_t fromlen = sizeof(from);

	return recvfrom(fd, buf, MAX_PACKET_LENGTH, 0,
			(struct sockaddr *) &from, &fromlen);
}

static void Usage(void)
{
	printf("Usage: \natalk_bridge\n\n");
	printf("atalk_bridge [-f fr] [-0 eth0] [-1 eth1] port\n\n");
	printf("fr = Ethernet frame type to bridge. Default is 802.2.\n");
	printf("     Known values are:\n");
	printf("         802.2 802.3 etherII echo x25 arp atalk aarp all\n");
	printf("if = Network interface to read from (default is eth0)\n");
}


void sig_int(int sig)
{
	eth_close(eth_fd0, if_eth0);
	eth_close(eth_fd1, if_eth1);
	printf("Wrote %d packets to eth0\n", eth0_counter);
	printf("Wrote %d packets to eth1\n", eth1_counter);
	printf("%d duplicate packets dropped\n", dup_counter);
	printf("%d IP packets processed\n", ip_counter);
	exit(0);
}


/* this is the guts of the program. A packet has arrived and we need to decide
   what to do with it */
static void process_packet(char *data, int len)
{
	int i;
	unsigned sum;
	struct ethhdr *hdr;
	unsigned protocol;

	if (len <= 0) return;

	sum = csum(data, len);

	/* work out where it is from */
	hdr = (struct ethhdr *)data;
	protocol = get_protocol(data);

#if DEBUG
	printf("packet %s -> %s len=%4d proto=%04x sum=%08x  ",
	       ether_str(hdr->h_source), 
	       ether_str(hdr->h_dest), 
	       len, protocol, sum);
#endif

	/* if the packet is from or to us then it makes no sense
	   to bridge it */
	if (memcmp(hdr->h_source, eth0_addr, ETH_ALEN) == 0 ||
	    memcmp(hdr->h_source, eth1_addr, ETH_ALEN) == 0 ||
	    memcmp(hdr->h_dest, eth0_addr, ETH_ALEN) == 0 ||
	    memcmp(hdr->h_dest, eth1_addr, ETH_ALEN) == 0) {
		/* its from us, drop it */
#if DEBUG
		printf("own packet\n");
#endif
		return;
	}

	/* don't process any ARP or RARP packets */
	if (protocol == ETH_P_ARP || protocol == ETH_P_RARP) {
#if DEBUG
		printf("ARP\n");
#endif
		return;
	}

	/* initially process IP packets to and from the hub, at
           least until the bridge gets established. Once the arp tables
	   get flushed we will just be able to drop all these packets */
	if (protocol == ETH_P_IP &&
	    memcmp(hdr->h_source, hub_addr, ETH_ALEN) == 0 &&
	    memcmp(hdr->h_dest,   hub_addr, ETH_ALEN) == 0) {
#if DEBUG
		printf("IP\n");
#endif
		return;
	}

	/* is it a duplicate? */
	for (i=0;i<HISTORY_LENGTH;i++)
		if (sum == history[i]) {
#if DEBUG
			printf("duplicate %d\n", i);
#endif
			history[i] = 0;
			dup_counter++;
			return;
		}
	history[history_pointer] = sum;
	history_pointer = (history_pointer+1) % HISTORY_LENGTH;

	if (protocol == ETH_P_IP) {
		printf("packet %s -> %s len=%4d proto=%04x sum=%08x\n",
		       ether_str(hdr->h_source), 
		       ether_str(hdr->h_dest), 
		       len, protocol, sum);
		ip_counter++;
	}
			
	if (memcmp(hdr->h_source, hub_addr, ETH_ALEN) == 0) {
		/* its from the hub (presumed to be on eth1). Send it to
		   eth0 */
#if DEBUG
		printf("to eth0\n");
#endif
		printf("packet %s -> %s len=%4d proto=%04x sum=%08x\n",
		       ether_str(hdr->h_source), 
		       ether_str(hdr->h_dest), 
		       len, protocol, sum);
		eth0_counter++;
		eth_write(eth_fd0, data, len, if_eth0);
	} else {
		/* its from a host on eth1, pass it on to eth1 */
#if DEBUG
		printf("to eth1\n");
#endif
		eth1_counter++;
		eth_write(eth_fd1, data, len, if_eth1);
	}
}


int main(int argc, char *argv[])
{
	int i;
	char *pstr = "all";

	if(getuid()) {
		fprintf(stderr, "must be superuser\n");
		exit(1);
	}

	fprintf(stderr,"atalk_bridge by Andrew Tridgell\n");

	/* Process command-line args */
	for(i=1; i<argc && argv[i][0]=='-'; ++i) switch(argv[i][1])
		{
		case 'f':
		   /* Selected frame types - from linux/if_ether.h */
		   pstr = argv[++i];
		   break;
		case '0': if_eth0 = argv[++i]; break;
		case '1': if_eth1 = argv[++i]; break;
		default:
			Usage();
			break;
		}

	parse_ether(hub_addr, HUB);
	parse_ether(eth0_addr, ETH0);
	parse_ether(eth1_addr, ETH1);

	eth_fd0 = eth_open(if_eth0, pstr);
	if (eth_fd0 == -1) {
		fprintf(stderr,"can't open %s\n", if_eth0);
		exit(1);
	}

	eth_fd1 = eth_open(if_eth1, pstr);
	if (eth_fd1 == -1) {
		fprintf(stderr,"can't open %s\n", if_eth1);
		eth_close(eth_fd0, if_eth0);
		exit(1);
	}

	signal(SIGINT, sig_int);

	printf("bridge started\n");

	while (1) {
		fd_set	rfd;
		int len, no;
		struct timeval tv;

		FD_ZERO(&rfd);
		FD_SET(eth_fd0, &rfd);
		FD_SET(eth_fd1, &rfd);

		tv.tv_sec = 2;
		tv.tv_usec = 0;

		no = select(32, &rfd, NULL, NULL, &tv);

		if (no == -1 && errno != EAGAIN) {
			printf("select error\n");
			break;
		}

		if (no == 0) {
			/* zero the history */
			memset(history, 0, sizeof(history));
			continue;
		}

		/* is anything happening on the ethernet? */
		if (FD_ISSET(eth_fd0, &rfd)) {
			len = eth_read(eth_fd0, data);
			process_packet(data, len);
		}

#if 0
		/* this is commented out as it seems that we get all
                   packets on both file sockets! */
		if (FD_ISSET(eth_fd1, &rfd)) {
			len = eth_read(eth_fd1, data);
			process_packet(data, len);
		}
#endif
	}

	return 0;
}
