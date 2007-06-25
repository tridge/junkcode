typedef struct {
	smb_ucs2_t lower;
	smb_ucs2_t upper;
	unsigned char flags;
} smb_unicode_table_t;


static smb_unicode_table_t map_table1[] = {
#include "unicode_map_table1.h"
};

static smb_unicode_table_t map_table2[] = {
#include "unicode_map_table2.h"
};

main()
{
	
}
