/* manipulate PCI devices from user space

   Tridge, July 2000
*/
#include "usbfan.h"

#define MAX_BUS 8

/* find a PCI device and return a handle to it */
int pci_find_device(u32 vendor, u32 device)
{
	int bus;
	DIR *dir;
	struct dirent *dent;
	char path[100];
	int fd;
	u16 ven, dev;

	for (bus=0;bus<MAX_BUS;bus++) {
		sprintf(path,"/proc/bus/pci/%02d", bus);
		dir = opendir(path);
		if (!dir) continue;
		while ((dent = readdir(dir))) {
			if (!isxdigit(dent->d_name[0])) continue;
			sprintf(path,"/proc/bus/pci/%02d/%s", bus, dent->d_name);
			fd = open(path,O_RDWR);
			if (fd == -1) {
				perror(path);
				continue;
			}
			if (pread(fd, &ven, 2, PCI_VENDOR_ID) == 2 &&
			    pread(fd, &dev, 2, PCI_DEVICE_ID) == 2 &&
			    ven == vendor && dev == device) {
				closedir(dir);
				return fd;
			}
			close(fd);
		}
		closedir(dir);
	}
	
	printf("failed to find pci device %x:%x\n", vendor, device);
	return -1;
}


/* routines to read and write PCI config space */
int pci_config_write_u8(int fd, int ofs, u8 v)
{
	return (pwrite(fd, &v, sizeof(v), ofs) == sizeof(v) ? 0 : -1);
}

int pci_config_write_u16(int fd, int ofs, u16 v)
{
	return (pwrite(fd, &v, sizeof(v), ofs) == sizeof(v) ? 0 : -1);
}

int pci_config_write_u32(int fd, int ofs, u32 v)
{
	return (pwrite(fd, &v, sizeof(v), ofs) == sizeof(v) ? 0 : -1);
}

int pci_config_read_u8(int fd, int ofs, u8 *v)
{
	return (pread(fd, v, sizeof(*v), ofs) == sizeof(*v) ? 0 : -1);
}

int pci_config_read_u16(int fd, int ofs, u16 *v)
{
	return (pread(fd, v, sizeof(*v), ofs) == sizeof(*v) ? 0 : -1);
}

int pci_config_read_u32(int fd, int ofs, u32 *v)
{
	return (pread(fd, v, sizeof(*v), ofs) == sizeof(*v) ? 0 : -1);
}


/* pinched from kernel pci.c */
int pci_find_capability(int fd, int cap)
{
	u16 status;
	u8 pos, id;
	int ttl = 48;
	u8 hdr_type;

	pci_config_read_u8(fd, PCI_HEADER_TYPE, &hdr_type);
	hdr_type &= 0x7f;

	printf("hdr_type=%d\n", hdr_type);

	pci_config_read_u16(fd, PCI_STATUS, &status);

	printf("status=0x%x\n", status);

	if (!(status & PCI_STATUS_CAP_LIST))
		return 0;
	switch (hdr_type) {
	case PCI_HEADER_TYPE_NORMAL:
	case PCI_HEADER_TYPE_BRIDGE:
		pci_config_read_u8(fd, PCI_CAPABILITY_LIST, &pos);
		printf("%d pos=%d\n", __LINE__, pos);
		break;
	case PCI_HEADER_TYPE_CARDBUS:
		pci_config_read_u8(fd, PCI_CB_CAPABILITY_LIST, &pos);
		break;
	default:
		return 0;
	}
	while (ttl-- && pos >= 0x40) {
		pos &= ~3;
		pci_config_read_u8(fd, pos + PCI_CAP_LIST_ID, &id);
		if (id == 0xff)
			break;
		if (id == cap)
			return pos;
		pci_config_read_u8(fd, pos + PCI_CAP_LIST_NEXT, &pos);
	}
	return 0;
}
