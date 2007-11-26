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
#include "readframes.h"

#define READ_SIZE 61440

static double total_time, min_time, max_time, total_bytes;

static void test_file(const char *fname)
{
	int fd;
	unsigned char buf[READ_SIZE];
	LARGE_INTEGER counter1, counter2, freq;
	double t;
	int n;
	HANDLE h;
	DWORD nread;

	QueryPerformanceCounter(&counter1);

        h = CreateFile (
                fname, GENERIC_READ, 
                FILE_SHARE_READ,
                NULL,           
                OPEN_EXISTING,  
                0,
                NULL
        );

	if (h == INVALID_HANDLE_VALUE) {
		printf("Failed to open %s\n", fname);
		return;
	}

	while (ReadFile(h, buf, READ_SIZE, &nread, NULL) && nread != 0) {
		total_bytes += nread;
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


static int num_files;
static char **file_list;

static void test_directory(const char *dname)
{
	WIN32_FIND_DATA list;
	HANDLE h;
	TCHAR dir[MAX_PATH+1];

	sprintf(dir, "%s\\*", dname);

	h = FindFirstFile(dir, &list);
	if (h == INVALID_HANDLE_VALUE) {
		printf("No files in %s\n", dname);
		return;
	}
	do {
		TCHAR fname[MAX_PATH+1];
		sprintf(fname, "%s\\%s", dname, list.cFileName);
		if (strcmp(list.cFileName, ".") == 0 ||
		    strcmp(list.cFileName, "..") == 0) continue;
		    
		if (list.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			test_directory(fname);
		} else {
			if (file_list == NULL) {
				file_list = malloc(sizeof(char *));
			} else {
				file_list = realloc(file_list, sizeof(char *)*(num_files+1));
			}
			file_list[num_files] = strdup(fname);
			num_files++;
		}
	} while (FindNextFile(h, &list));
	FindClose(h);
}

static int name_cmp(char **n1, char **n2)
{
	const char *s1=*n1, *s2=*n2;
	/* try to do numerical sorting */
	while (*s1 && *s1 == *s2) {
		s1++; s2++;
	}
	if (isdigit(*s1) || isdigit(*s2)) {
		return atoi(s1) - atoi(s2);
	}
	return strcmp(s1, s2);
}

int main(int argc, char* argv[])
{
	int i;	

	printf("readframes tester - tridge@samba.org\n");

	if (argc < 2) {
		printf("Usage: readframes <directory>\n");
		exit(1);
	}

	for (i=1;i<argc;i++) {
		printf("Testing on directory %s\n", argv[i]);
		test_directory(argv[i]);
	}

	qsort(file_list, num_files, sizeof(char *), name_cmp);
	
	for (i=0;i<num_files;i++) {
		test_file(file_list[i]);
	}

	printf("\nProcessed %d files totalling %.2f MBytes\n", 
	       num_files, total_bytes/(1024*1024));
	printf("Speed was %.2f files/sec\n", num_files/total_time);
	printf("Average speed was %.2fms per file\n", 1000.0*total_time/num_files);
	printf("Worst: %.2fms  Best: %.2fms\n", max_time*1000.0, min_time*1000.0);

        return 0;
}
