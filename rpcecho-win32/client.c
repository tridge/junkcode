/* 
   RPC echo client.

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

void main(int argc, char **argv)
{
	RPC_STATUS status;
	char *binding = NULL, *network_address = NULL;

	if (argc < 3) {
		printf("Usage: rpcechocli hostname cmd [args]\n\n");
		printf("Where hostname is the name of the host to connect to,\n");
		printf("and cmd is the command to execute with optional args:\n\n");
		printf("\taddone num\tAdd one to num and return the result\n");
		printf("\techodata size\tSend an array of size bytes and receive it back\n");
		printf("\tsinkdata size\tSend an array of size bytes\n");
		printf("\tsourcedata size\tReceive an array of size bytes\n");
		printf("\ttest\trun testcall\n");
		exit(0);
	}

	if (strcmp(argv[1], "localhost") != 0)
		network_address = argv[1];

	argc = argc - 2;
	argv = argv + 2;

	status = RpcStringBindingCompose(
		NULL, /* uuid */
		"ncacn_np",
		network_address,
		"\\pipe\\rpcecho",
		NULL, /* options */
		&binding);

	if (status) {
		printf("RpcStringBindingCompose returned %d\n", status);
		exit(status);
	}

	printf("Endpoint is %s\n", binding);

	status = RpcBindingFromStringBinding(
			binding,
			&rpcecho_IfHandle);

	if (status) {
		printf("RpcBindingFromStringBinding returned %d\n", status);
		exit(status);
	}

	RpcTryExcept {

		/* Add one to a number */

		if (strcmp(argv[0], "addone") == 0) {
			int arg, result;

			if (argc != 2) {
				printf("Usage: addone num\n");
				exit(1);
			}

			arg = atoi(argv[1]);

			AddOne(arg, &result);
			printf("%d + 1 = %d\n", arg, result);

			goto done;
		}

		/* Echo an array */

		if (strcmp(argv[0], "echodata") == 0) {
			int arg, i;
			char *indata, *outdata;

			if (argc != 2) {
				printf("Usage: echo num\n");
				exit(1);
			}

			arg = atoi(argv[1]);

			if ((indata = malloc(arg)) == NULL) {
				printf("Error allocating %d bytes for input\n", arg);
				exit(1);
			}

			if ((outdata = malloc(arg)) == NULL) {
				printf("Error allocating %d bytes for output\n", arg);
				exit(1);
			}

			for (i = 0; i < arg; i++)
				indata[i] = i & 0xff;

			EchoData(arg, indata, outdata);

			for (i = 0; i < arg; i++) {
				if (indata[i] != outdata[i]) {
					printf("data mismatch at offset %d, %d != %d\n",
						i, indata[i], outdata[i]);
					exit(0);
				}
			}

			goto done;
		}

		if (strcmp(argv[0], "sinkdata") == 0) {
			int arg, i;
			char *indata;

			if (argc != 2) {
				printf("Usage: sinkdata num\n");
				exit(1);
			}

			arg = atoi(argv[1]);		

			if ((indata = malloc(arg)) == NULL) {
				printf("Error allocating %d bytes for input\n", arg);
				exit(1);
			}			

			for (i = 0; i < arg; i++)
				indata[i] = i & 0xff;

			SinkData(arg, indata);

			goto done;
		}

		if (strcmp(argv[0], "sourcedata") == 0) {
			int arg, i;
			unsigned char *outdata;

			if (argc != 2) {
				printf("Usage: sourcedata num\n");
				exit(1);
			}

			arg = atoi(argv[1]);		

			if ((outdata = malloc(arg)) == NULL) {
				printf("Error allocating %d bytes for output\n", arg);
				exit(1);
			}			

			SourceData(arg, outdata);

			for (i = 0; i < arg; i++) {
				if (outdata[i] != (i & 0xff)) {
					printf("data mismatch at offset %d, %d != %d\n",
						i, outdata[i], i & 0xff);
				}
			}

			goto done;
		}

		if (strcmp(argv[0], "test") == 0) {
			printf("no TestCall\n");
			goto done;
		}

		printf("Invalid command '%s'\n", argv[0]);

	} RpcExcept(1) {
		unsigned long ex;

		ex = RpcExceptionCode();
		printf("Runtime error 0x%x\n", ex);
	} RpcEndExcept

done:

	status = RpcStringFree(&binding);

	if (status) {
		printf("RpcStringFree returned %d\n", status);
		exit(status);
	}

	status = RpcBindingFree(&rpcecho_IfHandle);

	if (status) {
		printf("RpcBindingFree returned %d\n", status);
		exit(status);
	}

	exit(0);
}
