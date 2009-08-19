/*
  raw meter reader for Linux

  (C) Andrew Tridgell 2009
  Released under GPLv3

  to compile:

    gcc RawMeterReader.c -o RawMeterReader.exe -lusb-1.0
 */
#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{

	libusb_context *ctx;
	unsigned short vendor_id  = 0x1244;
	unsigned short product_id = 0xd237;
	libusb_device_handle *h;
	int ret, nread;
	unsigned char data[14];
	unsigned char cdata[0x53];

	libusb_init(&ctx);
	libusb_set_debug(ctx, 3);
	
	h = libusb_open_device_with_vid_pid(ctx, vendor_id, product_id);
	if (h == NULL) {
		fprintf(stderr, "Failed to open DMM device\n");
		exit(1);
	}

	libusb_detach_kernel_driver(h, 0);

	ret = libusb_claim_interface(h, 0);
	if (ret != 0) {
		fprintf(stderr, "Failed to claim interface\n");
		exit(1);
	}

	memset(cdata, 0, sizeof(cdata));

	ret = libusb_control_transfer(h, 0x80, 0x06, 0x0200, 0, cdata, 0x22, 0); 
	ret = libusb_control_transfer(h, 0x80, 0x06, 0x0200, 0, cdata, 0x22, 0); 
	ret = libusb_control_transfer(h, 0x21, 0x0a, 0x0000, 0, cdata, 0, 0); 
	ret = libusb_control_transfer(h, 0x81, 0x06, 0x2200, 0, cdata, 0x53, 0); 

	while (1) {
		int i;

		memset(data, 0, sizeof(data));
		ret = libusb_interrupt_transfer(h, 0x81, data, sizeof(data), &nread, 10000);
		if (ret != 0 || nread != 14) {
			fprintf(stderr, "libusb_interrupt_transfer ret=%d nread=%d\n", ret, nread);
			continue;
		}
		printf("00 ");
		for (i=0;i<14;i++) {
			printf("%02X ", data[i]);
		}
		printf("\n");
		fflush(stdout);
	}
	
	return 0;
}
