// errmap.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include "ntsecapi.h"

static void nt_to_win32(void)
{
	NTSTATUS i;
	ULONG werror;
	printf("/* errmap NTSTATUS->Win32 */\n");

	for (i=1;i<0x10000; i++) {
		werror = LsaNtStatusToWinError(i);
		if (werror == ERROR_MR_MID_NOT_FOUND) continue;
		printf("{0x%x, 0x%x},\n", i, werror);
	}

	for (i=0xc0000000;i<0xc0010000; i++) {
		werror = LsaNtStatusToWinError(i);
		if (werror == ERROR_MR_MID_NOT_FOUND) continue;
		printf("{0x%x, 0x%x},\n", i, werror);
	}

	for (i=0x80000000;i<0x80010000; i++) {
		werror = LsaNtStatusToWinError(i);
		if (werror == ERROR_MR_MID_NOT_FOUND) continue;
		printf("{0x%x, 0x%x},\n", i, werror);
	}
}

static void dos_to_win32(void)
{
   DWORD werror;
   int i;
 
   printf("/* dos -> win32 error codes */\n");

	for (i=1;i<1024;i++) {
		char fname[100];
		sprintf(fname, "errmap.%d", i);
		DeleteFile(fname);
		werror = GetLastError();
		printf("{0x%x, 0x%x},\n", i, werror);
	}
}

int main(int argc, char *argv[])
{
	dos_to_win32();
	return 0;
}