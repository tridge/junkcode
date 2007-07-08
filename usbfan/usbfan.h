#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <dirent.h>
#include <ctype.h>
#include <linux/pci.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned u32;

#define PAGE_SIZE 0x1000

int pci_find_device(u32 vendor, u32 device);
int pci_config_write_u8(int fd, int ofs, u8 v);
int pci_config_write_u16(int fd, int ofs, u16 v);
int pci_config_write_u32(int fd, int ofs, u32 v);
int pci_config_read_u8(int fd, int ofs, u8 *v);
int pci_config_read_u16(int fd, int ofs, u16 *v);
int pci_config_read_u32(int fd, int ofs, u32 *v);
int pci_find_capability(int fd, int cap);

