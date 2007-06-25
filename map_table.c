#include <stdio.h>

#define smb_ucs2_t unsigned short
#define UNI_UPPER    0x1
#define UNI_LOWER    0x2
#define UNI_DIGIT    0x4
#define UNI_XDIGIT   0x8
#define UNI_SPACE    0x10

typedef struct {
	smb_ucs2_t lower;
	smb_ucs2_t upper;
	unsigned char flags;
} smb_unicode_table_t;

smb_unicode_table_t map_table[] = {
#include "unicode_map_table.h"
};


main()
{
	int i, count=0;
	for (i=0;i<65536;i++) {
		if (map_table[i].lower != map_table[i].upper) {
			printf("0x%04x %d\n", i, map_table[i].upper - map_table[i].lower);
			count++;
		}
	}
	printf("count=%d\n", count);
}
