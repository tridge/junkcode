#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

typedef unsigned char u8;

#define PAGE_SIZE 0x1000

static void scan_pes(u8 *p, size_t len)
{
	u8 *p0 = p;
	u8 *p1 = p;
	int nPacketLen, nStreamID, nDTS, nPTS, nHeaderLen;

	while (len > 6) {
		/* look for a PES start */
		while (len > 6 && (p[0] || p[1] || p[2] != 1)) { p++; len--; }

		nPacketLen = p[4] << 8 | p[5];
		nStreamID  = p[3];

		if (p[7] & 0x80) { /* PTS avail */
			nPTS  = (p[ 9] & 0x0E) << 29 ;
			nPTS |=  p[10]         << 22 ;
			nPTS |= (p[11] & 0xFE) << 14 ;
			nPTS |=  p[12]         <<  7 ;
			nPTS |= (p[13] & 0xFE) >>  1 ;
		} else {
			nPTS = 0;
		}
    
		if (p[7] & 0x40) { /* DTS avail */
			nDTS  = (p[14] & 0x0E) << 29 ;
			nDTS |=  p[15]         << 22 ;
			nDTS |= (p[16] & 0xFE) << 14 ;
			nDTS |=  p[17]         <<  7 ;
			nDTS |= (p[18] & 0xFE) >>  1 ;
		} else {
			nDTS = 0;
		}


		nHeaderLen = p[8];

		printf("offset=0x%x xsize=0x%x stream_id=0x%x len=0x%x headerlen=%d pts=0x%x dts=0x%x\n", 
		       (int)(p-p0), (int)(p-p1), nStreamID, nPacketLen, nHeaderLen, nPTS, nDTS);

		p1 = p;

		p    += nHeaderLen + 9;
		nPacketLen -= nHeaderLen + 3;
	}

}

int main(int argc, char *argv[])
{
	u8 *p;
	int fd;
	struct stat st;

	fd = open(argv[1], O_RDONLY);
	
	fstat(fd, &st);

	p = mmap(NULL, (st.st_size+(PAGE_SIZE-1))&~(PAGE_SIZE-1), PROT_READ, MAP_PRIVATE|MAP_FILE, fd, 0);
	
	scan_pes(p, st.st_size);
	return 0;
}
