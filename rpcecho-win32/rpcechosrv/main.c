/* 
   RPC echo server.

   Copyright (C) Tim Potter 2003
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "rpcecho.h"

#define RPC_MIN_CALLS 1
#define RPC_MAX_CALLS 20
#define RPC_ENDPOINT "\\pipe\\rpcecho"

void AddOne(int in_data, __RPC_FAR int *out_data)
{
	printf("AddOne: got in_data = %d\n", in_data);
	*out_data = in_data + 1;
}

void EchoData(int len, unsigned char __RPC_FAR in_data[],
	unsigned char __RPC_FAR out_data[])
{
	int i;

	printf("EchoData: got len = %d\n", len);
	
	for (i = 0; i < len; i++)
		out_data[i] = i & 0xff;
}

void SinkData(int len, unsigned char __RPC_FAR in_data[  ])
{
	printf("SinkData: got len = %d\n", len);
}

void SourceData(int len, unsigned char __RPC_FAR out_data[  ])
{
	int i;

	printf("SourceData: got len = %d\n", len);

	for (i = 0; i < len; i++)
		out_data[i] = i & 0xff;
}

void main(int argc, char **argv)
{
	RPC_STATUS status;

	if (argc != 1) {
		printf("Usage: rpcechosrv\n");
		exit(0);
	}

	status = RpcServerUseProtseqEp(
		"ncacn_np", RPC_MAX_CALLS, RPC_ENDPOINT,
		NULL);

	if (status)
		exit(status);

	status = RpcServerRegisterIf(
		rpcecho_v1_0_s_ifspec, NULL, NULL);

	if (status)
		exit(status);

	status = RpcServerListen(RPC_MIN_CALLS, RPC_MAX_CALLS, FALSE);

	if (status) {
		printf("RpcServerListen returned error %d\n", status);
		exit(status);
	}
}
