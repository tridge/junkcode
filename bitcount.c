typedef unsigned uint32;

static inline int bcount1(uint32 x)
{
	int count=0;
	int i;
	for (i=0;i<32;i++)
		if (x & (1<<i)) count++;
	return count;
}

static inline int bcount2(uint32 x)
{
	int count;
	for (count=0; x; count++)
		x &= (x-1);
	return count;
}


static int pop_count[256];

static void init_eval_tables(void)
{
	int i;

	for (i=0;i<256;i++) {
		int j, ret=0;
		for (j=0;j<8;j++)
			if (i & (1<<j))
				ret++;
		pop_count[i] = ret;
	}
}

static inline int pop_count16(uint32 x)
{
	return pop_count[(x)&0xFF] + pop_count[((x)>>8)&0xFF];
}

static inline int pop_count32(uint32 x)
{
	return pop_count16((x) & 0xFFFF) + pop_count16((x) >> 16);
}

int main(void)
{
	int i;
	uint32 x;

	init_eval_tables();

	for (i=0;i<1000000;i++) {
		x = random();
		if (pop_count32(x) != bcount1(x))
			printf("x=%x\n", x);
	}

	return 0;
}
