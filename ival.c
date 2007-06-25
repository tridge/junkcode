#include <stdio.h>
#include <string.h>

typedef unsigned short uint16;
typedef unsigned int uint32;

#ifndef X86
#define PVAL(buf,pos) ((((const unsigned char *)(buf))[pos]))
#define SVAL(buf,pos) ((PVAL(buf,pos)|PVAL(buf,(pos)+1)<<8))
#define IVAL(buf,pos) ((SVAL(buf,pos)|SVAL(buf,(pos)+2)<<16))
#else
#define SVAL(buf,pos) (*(const uint16 *)((const char *)(buf) + (pos)))
#define IVAL(buf,pos) (*(const uint32 *)((const char *)(buf) + (pos)))
#endif

#define SMB_BIG_UINT unsigned long long

static char buf[4];

int main(void)
{
	SMB_BIG_UINT x;
	
	memset(buf, 0xFF, 4);
	buf[3] = 0x7F;

	x = IVAL(buf, 0);

	printf("%.0f\n", (double)x);
	return 0;
}
