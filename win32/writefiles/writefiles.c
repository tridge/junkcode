/* 
   test reading image frames from a directory
   tridge@samba.org, March 2006
*/

#define _WIN32_WINNT 0x0400

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include "windows.h"
#include "winbase.h"
#include "writefiles.h"

#define WRITE_SIZE 61440

static double total_time, min_time, max_time, total_bytes;

#define MIN(a,b) ((a)<(b)?(a):(b))

static void test_file(const char *fname, unsigned long filesize)
{
	int fd;
	unsigned char buf[WRITE_SIZE];
	LARGE_INTEGER counter1, counter2, freq;
	double t;
	int n;
	HANDLE h;
	DWORD nwritten;

	QueryPerformanceCounter(&counter1);

        h = CreateFile (
                fname, GENERIC_WRITE, 
                FILE_SHARE_READ|FILE_SHARE_WRITE,
                NULL,           
                CREATE_ALWAYS,  
                0,
                NULL
        );

	if (h == INVALID_HANDLE_VALUE) {
		printf("Failed to open %s\n", fname);
		return;
	}

	memset(buf, 0x42, WRITE_SIZE);

	while (WriteFile(h, buf, MIN(WRITE_SIZE, filesize), &nwritten, NULL) && 
	       nwritten != 0 &&
	       filesize >= 0) {
		total_bytes += nwritten;
		filesize -= nwritten;
	}
	CloseHandle(h);
	QueryPerformanceCounter(&counter2);

	QueryPerformanceFrequency(&freq);

	t = (1.0*counter2.QuadPart - 1.0*counter1.QuadPart) / (1.0*freq.QuadPart);

	if (t > max_time) max_time = t;
	if (min_time == 0 || t < min_time) min_time = t;
	total_time += t;

	printf("%6.2fms %s\n", t*1000.0, fname);
}


int main(int argc, char* argv[])
{
	unsigned i;	
	const char *dir;
	unsigned long filesize;
	unsigned numfiles;

	printf("writefiles tester - tridge@samba.org\n");

	if (argc < 4) {
		printf("Usage: writefiles <directory> <filesize> <numfiles>\n");
		exit(1);
	}

	dir = argv[1];
	filesize = strtoul(argv[2], NULL, 0);
	numfiles = strtoul(argv[3], NULL, 0);

	printf("writing %u files in %s\n", numfiles, dir);

	for (i=0;i<numfiles;i++) {
		char fname[1024];
		sprintf(fname, "%s\\file%u.dat", dir, i);
		test_file(fname, filesize);
	}

	printf("\nProcessed %d files totalling %.2f MBytes at %.2f MByte/s\n", 
	       numfiles, total_bytes/(1024*1024), total_bytes/(total_time*1024*1024));
	printf("Speed was %.2f files/sec\n", numfiles/total_time);
	printf("Average speed was %.2fms per file\n", 1000.0*total_time/numfiles);
	printf("Worst: %.2fms  Best: %.2fms\n", max_time*1000.0, min_time*1000.0);

        return 0;
}
