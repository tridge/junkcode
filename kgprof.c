#include <stdio.h>
#include <string.h>

#define MAX_MAPSIZE 10000

static struct {
	unsigned addr;
	char *name;
} sysmap[MAX_MAPSIZE];

static int mapsize;

static void load_map(void)
{
	FILE *f;
	unsigned addr;
	char dum[100];
	char fn[100];

	f = fopen("/boot/System.map", "r");
	while (!feof(f) && mapsize < MAX_MAPSIZE) {
		if (fscanf(f,"%x %s %s", &addr, dum, fn) == 3) {
			sysmap[mapsize].addr = addr;
			sysmap[mapsize].name = strdup(fn);
			mapsize++;
		}
	}
	fclose(f);
}

static char *find_map(unsigned addr)
{
	int low, high, i;

	low = 0;
	high = mapsize-1;

	while (low != high) {
		i = (low+high)/2;
		if (addr >= sysmap[i].addr) {
			low = i;
		}
		if (addr < sysmap[i].addr) {
			high = i-1;
		} 
		if (addr >= sysmap[i+1].addr) {
			low = i+1;
		}
		if (addr < sysmap[i+1].addr) {
			high = i;
		}
	}

	return sysmap[i].name;
}

static void disp_one(char *line)
{
	unsigned addr[6];
	unsigned t, count;
	char fname[30];
	int i;

	sscanf(line,"%s %u %u %x %x %x %x %x %x",
	       fname, &count, &t, 
	       &addr[0], &addr[1], &addr[2], 
	       &addr[3], &addr[4], &addr[5]);

	printf("%s %u %u:", fname, count, t);
	for (i=0;i<6;i++) {
		printf(" %s", find_map(addr[i]));
	}
	printf("\n");
}

int main()
{
	char line[1000];
	int enabled = 0;
	FILE *f;

	load_map();

	printf("loaded map\n");

	f = fopen("/proc/cpuinfo", "r");

	while (fgets(line, sizeof(line)-1, f)) {
		if (enabled) {
			disp_one(line);
		}
		if (strncmp(line,"kgprof", 6) == 0) enabled = 1;
	}

}
