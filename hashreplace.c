#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>


#define TOTAL_SIZE (1024*1024)
#define HASH_SIZE (1000)
#define NBUCKETS 8

typedef unsigned u32;

struct hash_entry {
	u32 offset;
	u32 tag;
};



static struct hash_entry hash_table[HASH_SIZE];

static int bucket_flat(u32 offset, u32 limit)
{
	u32 b = (offset / (limit/NBUCKETS));
	return NBUCKETS - b;
}

static int bucket(u32 offset, u32 limit)
{
	u32 rem = limit-offset;
	u32 m = limit>>NBUCKETS;
	int i = 0;
	u32 b = m;

	if (m == 0) return 0;

	while (rem > b) {
		i++;
		b += (m<<i);
	}

	return i;
}


static void print_distribution(u32 limit)
{
	u32 counts[NBUCKETS+1];
	int i;
	memset(counts, 0, sizeof(counts));

	for (i=0;i<HASH_SIZE;i++) {
		u32 b = bucket(hash_table[i].offset, limit);
		counts[b]++;
	}

	for (i=0;i<NBUCKETS+1;i++) {
		printf("bucket %3d  %.3f\n", 
		       i, (100.0 * counts[i]) / HASH_SIZE);
	}
}


static int replace(u32 old_offset, u32 new_offset)
{
	u32 x;

	x = 1.2 * sqrt(new_offset * old_offset) / HASH_SIZE;

	if (x == 0) return 1;

	if (random() % x == 0) return 1;
	return 0;
}

static u32 lbs(u32 x)
{
	u32 i;
	for (i=0;i<32;i++) {
		if (x & (1<<i)) break;
	}
	return i;
}

static int yy_replace(u32 old_offset, u32 new_offset)
{
	u32 x = 20;

	if (1.0*lbs(old_offset) < (bucket(old_offset, new_offset)+1)) {
		return 1;
	}

	return 0;
}

static int XX_replace(u32 old_offset, u32 new_offset)
{
	double p;
	p = 0.5 / (((double)new_offset) / (HASH_SIZE * NBUCKETS));
	return (random() % 1000) < 1000 * p;
}

static void fill_hash_table(void)
{
	int i;
	for (i=0;i<TOTAL_SIZE/2;i++) {
		u32 tag = random();
		u32 t = tag % HASH_SIZE;

		if (hash_table[t].offset == 0 ||
		    replace(hash_table[t].offset, i)) {
			hash_table[t].offset = i;
			hash_table[t].tag = tag;
		}
	}

	print_distribution(i);
	printf("\n");

	for (;i<TOTAL_SIZE;i++) {
		u32 tag = random();
		u32 t = tag % HASH_SIZE;

		if (hash_table[t].offset == 0 ||
		    replace(hash_table[t].offset, i)) {
			hash_table[t].offset = i;
			hash_table[t].tag = tag;
		}
	}

	print_distribution(i);
}

 int main(int argc, const char *argv[])
{
	srandom(time(NULL));
	fill_hash_table();
	return 0;
}
