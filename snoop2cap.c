/* convert a snoop capture file to a netmon .cap file.
     by Seiichi Tatsukawa (stat@rational.com), March 1998.

   Based on capconvert.c by Andrew Tridgell, October 1997

   This only works for 32 bit boxes at the moment. Change the typedefs
   to work on other sytems
*/

#include <stdlib.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <memory.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

/* change the following 3 lines to make things work on
   systems with different word sizes */
typedef int int32;
typedef unsigned int uint32;
typedef unsigned short uint16;


/* #define _DEBUG 1 */

#define SNOOP_MAGIC "snoop\0\0\0"

#ifndef WIN32
#define	O_BINARY	0
#else
#pragma pack(push)
#pragma pack(1)

struct timeval {
	uint32	tv_sec;		/* seconds */
	int32	tv_usec;	/* and microseconds */
};
#endif

/* snoop file format */
struct snoop_file_header {
	union {
		uint32 m[2];
		char magic[8];
	} u;
	uint16 major;
	uint16 minor;
	uint32 linktype;
};

struct snoop_packet {
	uint32 caplen;
	uint32 len;
	uint32 offset;
	uint32 pad1;
	struct timeval ts;
};


/* .cap file format */
typedef struct _systemtime {
    uint16 wYear;
    uint16 wMonth; 
    uint16 wDayOfWeek;
    uint16 wDay;
    uint16 wHour;
    uint16 wMinute; 
    uint16 wSecond;
    uint16 wMilliseconds;
} systemtime_t;

struct cap_header {
    char             rtss[4];
    char             minor;
    char             major;
    uint16           captype; 
    systemtime_t     starttime;
    uint32           frameoffset;
    uint32           framelength;
    uint32           unknown[24];
};

struct cap_packet {
	uint32 cap_time; /* milliseconds */
	uint16 len;
	uint16 caplen;
};

#ifdef WIN32
#pragma pack(pop)
#endif

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
	swap_uint16(&h->starttime.wYear);
	swap_uint16(&h->starttime.wMonth);
	swap_uint16(&h->starttime.wDayOfWeek);
	swap_uint16(&h->starttime.wDay);
	swap_uint16(&h->starttime.wHour);
	swap_uint16(&h->starttime.wMinute);
	swap_uint16(&h->starttime.wSecond);
	swap_uint16(&h->starttime.wMilliseconds);
	swap_uint32(&h->frameoffset);
	swap_uint32(&h->framelength);
}

static void swap_netmon_packet(struct cap_packet *p)
{
	swap_uint32(&p->cap_time);
	swap_uint16(&p->len);
	swap_uint16(&p->caplen);
}

static void swap_snoop_header(struct snoop_file_header *h)
{
	swap_uint16(&h->major);
	swap_uint16(&h->minor);
	swap_uint32(&h->linktype);
}

static void swap_snoop_packet(struct snoop_packet *p)
{
	swap_uint32(&p->caplen);
	swap_uint32(&p->len);
	swap_uint32(&p->offset);
	swap_uint32(&((p->ts).tv_sec));
	swap_int32(&((p->ts).tv_usec));
}

int convert_snoop_to_cap(char *infile, char *outfile)
{
	int i;
	int fd1, fd2;
	int snaplen;
        int offsetlen;
	struct snoop_file_header snoop_header;
	struct snoop_packet snoop_pkt;
	char *data;
	struct cap_header cap_header;
	struct cap_packet cap_pkt;
	struct timeval tval1;
	char pad[4];
	int swap_snoop = 0;
	int swap_netmon = 0;
        struct tm *starttm;
	uint32 foffset;
	FILE *tmpf;
	int fd3;
	int j;

	fd1 = open(infile, O_RDONLY | O_BINARY);
	if (fd1 == -1) {
		perror(infile);
		exit(1);
	}

	fd2 = open(outfile, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
	if (fd2 == -1) {
		perror(outfile);
		exit(1);
	}

	if ((tmpf = tmpfile()) == NULL) {
		perror("tmpfile()");
		exit(1);
	}
	fd3 = fileno(tmpf);
	if (fd3 == -1) {
		perror("tmpfile()");
		exit(1);
	}

	if (read(fd1, &snoop_header, sizeof(snoop_header)) != 
	    sizeof(snoop_header)) {
		printf("can't read snoop header\n");
		return 0;
	}

	/* snoop files can be in either byte order. */
	if (memcmp(snoop_header.u.magic, SNOOP_MAGIC, 8) != 0) {
		printf("bad magic number %x\n", snoop_header.u.magic);
		return 0;
	}

	if (snoop_header.u.m[1] == 'p') {
		swap_snoop = 1;
		swap_snoop_header(&snoop_header);
#if _DEBUG
		fprintf(stderr, "Bytes swapping snoop file.\n");
#endif
	}

	/* check if we are little endian. If not, we need to
	   convert the network monitor output */
	i=1;
	if (((char *)&i)[0] != 1) {
		swap_netmon = 1;
#if _DEBUG
		fprintf(stderr, "Bytes swapping cap file.\n");
#endif
        }

	/* setup the basic netmon header */
	memset(&cap_header, 0, sizeof(cap_header));
	strcpy(cap_header.rtss, "RTSS");
	cap_header.minor = 1;
	cap_header.major = 1;
	cap_header.captype = 1; /* snoop_header.linktype; */
	
	/* write it out. we will have to write it again once
	   we've worked out the rest of the parameters */
	write(fd2, &cap_header, sizeof(cap_header));

	/* snaplen = snoop_header.snaplen; */
        snaplen = 4500;

	printf("snoop-%d.%d linktype=%d\n", 
	       snoop_header.major, snoop_header.minor,
	       snoop_header.linktype);

	data = (char *)malloc(snaplen);

	memset(pad, 0, sizeof(pad));
	
	for (i=0; 1 ; i++) {
		if (read(fd1, &snoop_pkt, sizeof(snoop_pkt)) != 
		    sizeof(snoop_pkt))
			break;

		if (swap_snoop)
                    swap_snoop_packet(&snoop_pkt);

		if (i == 0)
			tval1 = snoop_pkt.ts;

		if (read(fd1, data, snoop_pkt.caplen) != snoop_pkt.caplen)
			break;

#if _DEBUG
		fprintf(stderr, "frame %d of length=%d:%d\n",
			i+1, snoop_pkt.caplen, snoop_pkt.len);
#endif
                offsetlen = snoop_pkt.offset -
                    (sizeof(snoop_pkt) + snoop_pkt.caplen);
                if (offsetlen > 0)
                    lseek(fd1, offsetlen, SEEK_CUR);

		if (snoop_pkt.caplen > snoop_pkt.len)
			snoop_pkt.caplen = snoop_pkt.len;

		foffset = lseek(fd2, 0, SEEK_CUR);

		cap_pkt.cap_time = (snoop_pkt.ts.tv_sec - tval1.tv_sec)*1000 +
			(snoop_pkt.ts.tv_usec - tval1.tv_usec)/1000;
		cap_pkt.caplen = snoop_pkt.caplen;
		cap_pkt.len = snoop_pkt.len;

		if (swap_netmon)
			swap_netmon_packet(&cap_pkt);

		write(fd2, &cap_pkt, sizeof(cap_pkt));
		write(fd2, data, snoop_pkt.caplen);
		write(fd3, &foffset, sizeof(foffset));

		if (snoop_pkt.caplen % 4 != 0) {
			write(fd2, pad, 4 - (snoop_pkt.caplen % 4));
		}

#if _DEBUG
		fprintf(stderr, "frame %d of length=%d:%d\n",
			i+1, snoop_pkt.caplen, snoop_pkt.len);
#endif
	}

	cap_header.frameoffset = lseek(fd2, 0, SEEK_CUR);
	cap_header.framelength = i*4;

	if ((starttm = localtime((time_t *)&tval1.tv_sec)) != NULL) {
		cap_header.starttime.wYear = 1900 + starttm->tm_year;
		cap_header.starttime.wMonth = starttm->tm_mon + 1;
		cap_header.starttime.wDayOfWeek = starttm->tm_wday;
		cap_header.starttime.wDay = starttm->tm_mday;
		cap_header.starttime.wHour = starttm->tm_hour;
		cap_header.starttime.wMinute = starttm->tm_min;
		cap_header.starttime.wSecond = starttm->tm_sec;
		cap_header.starttime.wMilliseconds = tval1.tv_usec/1000;
	}

	if (swap_netmon)
		swap_netmon_header(&cap_header);

	lseek(fd3, 0, SEEK_SET);
	for (j = 0; j < i; j++) {
		if (read(fd3, &foffset, sizeof(foffset)) != sizeof(foffset)) {
			perror("read(tmpfile)");
			exit(2);
		}
		if (swap_netmon)
			swap_uint32(&foffset);
		write(fd2, &foffset, sizeof(foffset));
	}

	lseek(fd2, 0, SEEK_SET);

	write(fd2, &cap_header, sizeof(cap_header));

	close(fd1);
	close(fd2);
	fclose(tmpf);

	printf("converted %d frames\n", i);

	return i;
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		printf("Convert a snoop capture file to a netmon .cap file\n");
		printf("\tUsage: capconvert <infile> <outfile>\n");
		exit(1);
	}

	convert_snoop_to_cap(argv[1], argv[2]);

	return 0;
}
