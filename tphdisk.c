/*
  this creates a save2dsk.bin hibernation file for phoenix netbios
  laptops. I tested it on my IBM Thinkpad T20 but it should work on
  other laptops.

  Copyright 2002 Andrew Tridgell <tridge@samba.org> 
  released under the GNU General Public License version 2 or later
*/

/*
  This program was inspired by lphdisk which was written by Patrick
  Ashmore and Alex Stewart. The main difference is that tphdisk
  doesn't muck about with partition tables, which makes it easy to use
  it for either a hibernation partition or a save2dsk.bin file.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef unsigned short u16;
typedef unsigned u32;

/*
  write or die ...
*/
static void x_write(int fd, void *data, size_t size)
{
	if (write(fd, data, size) != (ssize_t)size) {
		fprintf(stderr,"Write failed!\n");
		exit(1);
	}
}

/* 
   the main function. The 'size' is in sectors (512 byte units) 

   this writes a save2dsk.bin file to the given file descriptor
*/
static void write_phdisk(int fd, u32 size)
{
	unsigned char sector[512];
	u16 checksum;
	u32 i;

	memset(sector, 0, 512);

	/* form the header - this is x86 byte order dependent, but who
	   has a non-intel lapptop with a phoenix notebios? */
	strcpy(sector, "TimO");
	sector[12] = 2;
	*(u32 *)&sector[16] = size;
	for (checksum=0, i=8;i<512;i+=2) {
		checksum += *(u16 *)(&sector[i]);
	}
	*(u16 *)&sector[6] = ~checksum;

	/* write two copies of the header */
	x_write(fd, sector, 512);
	x_write(fd, sector, 512);

	/* rest is filled with 0x50 */
	memset(sector, 0x50, 512);

	for (i=2;i<size;i++) {
		x_write(fd, sector, 512);
	}
}

/* main program */
int main(int argc, char *argv[])
{
	int size;

	if (argc < 2) {
		fprintf(stderr,"
Usage: tphdisk <size in MB>

written by Andrew Tridgell <tridge@samba.org>


This program writes a 'save2dsk.bin' hibernation file to stdout. To
use it you should do something like this:

1) create a type 16 (Hidden FAT16) partition on your laptop
2) format the partition with 'mkdosfs'
3) mount the partition as VFAT
4) create the 'save2dsk.bin' file on the partition using something like
      tphdisk 280 > save2dsk.bin
5) Do a full reboot

The only parameter is the size in megabytes of the save file. This
needs to be at least as big as your main memory + video memory, but you
can make it larger if you want to.
");
		exit(1);
	}

	size = atoi(argv[1]) * 1024 * 2;

	write_phdisk(1, size);
	return 0;
}
