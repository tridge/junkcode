/* convert a tcpdump capture file to a netmon .cap file.

   Andrew Tridgell, October 1997

   This only works for 32 bit boxes at the moment. Change the typedefs
   to work on other sytems
*/

#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <sys/time.h>
#include <stdio.h>
#include <fcntl.h>

/* change the following 3 lines to make things work on
   systems with different word sizes */
typedef int int32;
typedef unsigned int uint32;
typedef unsigned short uint16;


#define DEBUG 0

#define TCPDUMP_MAGIC 0xa1b2c3d4

/* tcpdump file format */
struct tcpdump_file_header {
	uint32 magic;
	uint16 major;
	uint16 minor;
	int32 zone;
	uint32 sigfigs;
	uint32 snaplen;
	uint32 linktype;
};

struct tcpdump_packet {
	struct timeval ts;
	uint32 caplen;
	uint32 len;
};


/* .cap file format */
struct cap_header {
    char             rtss[4];
    char             minor;
    char             major;
    uint16           captype; 
    char             starttime[16];
    uint32           frameoffset;
    uint32           framelength;
    uint32           unknown[24];
};

struct cap_packet {
	uint32 cap_time; /* milliseconds */
	uint16 len;
	uint16 caplen;
};

/* file format is:
   header
   frames
   frameoffsets
   */


static void swap_uint32(uint32 *x)
{
	char c, *p = (char *)x;
	c = p[0]; p[0] = p[3]; p[3] = c;
	c = p[1]; p[1] = p[2]; p[2] = c;
}

static void swap_int32(int32 *x)
{
	char c, *p = (char *)x;
	c = p[0]; p[0] = p[3]; p[3] = c;
	c = p[1]; p[1] = p[2]; p[2] = c;
}

static void swap_uint16(uint16 *x)
{
	char c, *p = (char *)x;
	c = p[0]; p[0] = p[1]; p[1] = c;
}


static void swap_netmon_header(struct cap_header *h)
{
	swap_uint16(&h->captype);
	swap_uint32(&h->frameoffset);
	swap_uint32(&h->framelength);
}

static void swap_netmon_packet(struct cap_packet *p)
{
	swap_uint32(&p->cap_time);
	swap_uint16(&p->len);
	swap_uint16(&p->caplen);
}

static void swap_tcpdump_header(struct tcpdump_file_header *h)
{
	swap_uint32(&h->magic);
	swap_uint16(&h->major);
	swap_uint16(&h->minor);
	swap_int32(&h->zone);
	swap_uint32(&h->sigfigs);
	swap_uint32(&h->snaplen);
	swap_uint32(&h->linktype);
}

static void swap_tcpdump_packet(struct tcpdump_packet *p)
{
	swap_uint32(&p->ts.tv_sec);
	swap_uint32(&p->ts.tv_usec);
	swap_uint32(&p->len);
	swap_uint32(&p->caplen);
}


int convert_tcpdump_to_cap(char *infile, char *outfile)
{
	uint32 *framestart = NULL;
	int maxframes = 1000000;
	int i;
	int fd1, fd2, fd3;
	int snaplen;
	struct tcpdump_file_header tcpdump_header;
	struct tcpdump_packet tcpdump_pkt;
	char *data;
	struct cap_header cap_header;
	struct cap_packet cap_pkt;
	struct timeval tval1;
	char pad[4];
	int swap_tcpdump = 0;
	int swap_netmon = 0;

	fd1 = open(infile, O_RDONLY);
	if (fd1 == -1) {
		perror(infile);
		exit(1);
	}

	fd2 = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd2 == -1) {
		perror(outfile);
		exit(1);
	}

	fd3 = open("cap.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd3 == -1) {
		perror("cap.dat");
		exit(1);
	}

	if (read(fd1, &tcpdump_header, sizeof(tcpdump_header)) != 
	    sizeof(tcpdump_header)) {
		printf("can't read tcpdump header\n");
		return 0;
	}

	write(fd3, &tcpdump_header, sizeof(tcpdump_header));

	/* tcpdump files can be in either byte order. */
	if (tcpdump_header.magic != TCPDUMP_MAGIC) {
		swap_tcpdump = 1;
		swap_tcpdump_header(&tcpdump_header);
	}

	if (tcpdump_header.magic != TCPDUMP_MAGIC) {
		printf("bad magic number %x\n", tcpdump_header.magic);
		return 0;
	}

	/* check if we are little endian. If not, we need to
	   convert the network monitor output */
	i=1;
	if (((char *)&i)[0] != 1) swap_netmon = 1;

	/* setup the basic netmon header */
	memset(&cap_header, 0, sizeof(cap_header));
	strcpy(cap_header.rtss, "RTSS");
	cap_header.minor = 1;
	cap_header.major = 1;
	cap_header.captype = tcpdump_header.linktype;
	
	/* write it out. we will have to write it again once
	   we've worked out the rest of the parameters */
	write(fd2, &cap_header, sizeof(cap_header));

	snaplen = tcpdump_header.snaplen;

	printf("tcpdump-%d.%d snaplen=%d linktype=%d\n", 
	       tcpdump_header.major, tcpdump_header.minor,
	       snaplen, tcpdump_header.linktype);

	data = (char *)malloc(snaplen);

	framestart = (uint32 *)malloc(maxframes * sizeof(framestart[0]));

	if (!framestart) {
		perror("malloc");
		exit(1);
	}

	memset(pad, 0, sizeof(pad));
	
	for (i=0; 1 ; i++) {
		if (i == maxframes-1) {
			framestart = (uint32 *)realloc(framestart, 
						       maxframes*2*
						       sizeof(framestart[0]));
			maxframes *= 2;
			if (!framestart) {
				perror("malloc");
				exit(1);
			}
		}

		if (read(fd1, &tcpdump_pkt, sizeof(tcpdump_pkt)) != 
		    sizeof(tcpdump_pkt))
			break;

		write(fd3, &tcpdump_pkt, sizeof(tcpdump_pkt));

		if (swap_tcpdump)
			swap_tcpdump_packet(&tcpdump_pkt);

		if (i == 0)
			tval1 = tcpdump_pkt.ts;

#if DEBUG
		printf("frame %d of length=%d:%d\n",
		       i+1, tcpdump_pkt.caplen, tcpdump_pkt.len);
#endif

		lseek(fd1, 8, SEEK_CUR);

		if (read(fd1, data, tcpdump_pkt.caplen) != tcpdump_pkt.caplen)
			break;

		write(fd3, data, tcpdump_pkt.caplen);

		if (tcpdump_pkt.caplen > tcpdump_pkt.len)
			tcpdump_pkt.caplen = tcpdump_pkt.len;

		framestart[i] = lseek(fd2, 0, SEEK_CUR);

		cap_pkt.cap_time = (tcpdump_pkt.ts.tv_sec - tval1.tv_sec)*1000 +
			(tcpdump_pkt.ts.tv_usec - tval1.tv_usec)/1000;
		cap_pkt.caplen = tcpdump_pkt.caplen;
		cap_pkt.len = tcpdump_pkt.len;

		if (swap_netmon)
			swap_netmon_packet(&cap_pkt);

		write(fd2, &cap_pkt, sizeof(cap_pkt));
		write(fd2, data, tcpdump_pkt.caplen);

		if (tcpdump_pkt.caplen % 4 != 0) {
			write(fd2, pad, 4 - (tcpdump_pkt.caplen % 4));
		}

#if DEBUG
		printf("frame %d of length=%d:%d\n",
		       i+1, tcpdump_pkt.caplen, tcpdump_pkt.len);
#endif
	}

	cap_header.frameoffset = lseek(fd2, 0, SEEK_CUR);
	cap_header.framelength = i*4;

	if (swap_netmon) {
		int j;
		for (j=0;j<i;j++)
			swap_uint32(&framestart[j]);
		swap_netmon_header(&cap_header);
	}

	write(fd2, framestart, i*4);

	lseek(fd2, 0, SEEK_SET);

	write(fd2, &cap_header, sizeof(cap_header));

	close(fd1);
	close(fd2);
	close(fd3);

	printf("converted %d frames\n", i);

	return i;
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		printf("Convert a tcpdump capture file to a netmon .cap file\n");
		printf("\tUsage: capconvert <infile> <outfile>\n");
		exit(1);
	}

	convert_tcpdump_to_cap(argv[1], argv[2]);

	return 0;
}
