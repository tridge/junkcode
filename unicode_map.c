#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define smb_ucs2_t unsigned short

typedef struct {
	smb_ucs2_t lower;
	smb_ucs2_t upper;
	unsigned char flags;
} smb_unicode_table_t;

#define UNI_UPPER    0x1
#define UNI_LOWER    0x2
#define UNI_DIGIT    0x4
#define UNI_XDIGIT   0x8
#define UNI_SPACE    0x10

static smb_unicode_table_t map_table[] = {
#include "unicode_map_table.h"
};


struct map_range {
	unsigned short start;
	unsigned short end;
	signed short lower_ofs;
	signed short upper_ofs;
	unsigned char flags;
};

static struct map_range range[0x10000];
static int map_range_size;

static unsigned short map_range_find(smb_ucs2_t val)
{
	unsigned low = 0, high = map_range_size-1;
	unsigned t;

	while (low != high) {
		t = (1+low+high)>>1;
		if (range[t].start > val) {
			high = t-1;
		} else {
			low = t;
		}
	}

	return low;
}

#define map_range_flags(val) (range[map_range_find(val)].flags)
#define map_range_lower(val) (range[map_range_find(val)].lower_ofs+(val))
#define map_range_upper(val) (range[map_range_find(val)].upper_ofs+(val))

static void build_map_range(void)
{
	int i, n;

	n = 0;
	range[n].start = 0;
	range[n].lower_ofs = map_table[0].lower;
	range[n].upper_ofs = map_table[0].upper;
	range[n].flags =     map_table[0].flags;

	for (i=1;i<0x10000;i++) {
		if (range[n].lower_ofs + i == map_table[i].lower &&
		    range[n].upper_ofs + i == map_table[i].upper &&
		    range[n].flags         == map_table[i].flags) continue;

		range[n].end = i-1;
		n++;
		range[n].start = i;
		range[n].lower_ofs = map_table[i].lower - i;
		range[n].upper_ofs = map_table[i].upper - i;
		range[n].flags =     map_table[i].flags;
	}
	range[n].end = i-1;
	map_range_size = n+1;
	printf("map_range_size = %d\n", map_range_size);
}


static void test_map_range(void)
{
	int i;

	for (i=0;i<0x10000;i++) {
		smb_ucs2_t v = (smb_ucs2_t)i;
		if (map_range_flags(v) != map_table[v].flags) {
			printf("flags failed at %d\n", i);
			break;
		}
		if (map_range_upper(v) != map_table[v].upper) {
			printf("upper failed at %d\n", i);
			break;
		}
		if (map_range_lower(v) != map_table[v].lower) {
			printf("lower failed at %d\n", i);
			break;
		}
	}
	
}


static struct timeval tp1,tp2;

static void start_timer()
{
	gettimeofday(&tp1,NULL);
}

static double end_timer()
{
	gettimeofday(&tp2,NULL);
	return((tp2.tv_sec - tp1.tv_sec) + 
	       (tp2.tv_usec - tp1.tv_usec)*1.0e-6);
}

static void speed_test(void)
{
	#define TEST_SIZE (1000*1000)
	smb_ucs2_t testbuf[TEST_SIZE];
	int i;
	unsigned sum;

	srandom(time(NULL));

	for (i=0;i<TEST_SIZE;i++) {
		testbuf[i] = random();
	}

	sum = 0;

	for (i=0;i<TEST_SIZE;i++) {
		smb_ucs2_t v = testbuf[i];
		sum += map_table[v].flags;
	}	
	start_timer();
	for (i=0;i<TEST_SIZE;i++) {
		smb_ucs2_t v = testbuf[i];
		sum += map_table[v].flags;
	}	
	printf("table took %g secs (sum=%d)\n", end_timer(), sum);

	sum = 0;

	for (i=0;i<TEST_SIZE;i++) {
		smb_ucs2_t v = testbuf[i];
		sum += map_range_flags(v);
	}	
	start_timer();
	for (i=0;i<TEST_SIZE;i++) {
		smb_ucs2_t v = testbuf[i];
		sum += map_range_flags(v);
	}	
	printf("range took %g secs (sum=%d)\n", end_timer(), sum);
}

main()
{
	build_map_range();
	test_map_range();
	speed_test();
}
