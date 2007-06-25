#define SIZE (2*1024*1024)

void main(void)
{
	unsigned *p = (char *)malloc(SIZE);
	char sum=0;
	memset(p,42,SIZE);

	while (1) {
		int i;
		for (i=0;i<SIZE/4;i++) sum += p[i];
	}
}
