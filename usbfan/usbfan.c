#include "usbfan.h"

#define RH_PORT_POWER		0x08
#define USBPORTSC1	16
#define USBPORTSC2	18

#define   USBCMD_EGSM		0x0008	/* Global Suspend Mode */
#define   USBCMD_FGR		0x0010	/* Force Global Resume */
#define   USBCMD		0
#define   USBCMD_GRESET		0x0004	/* Global reset */

int main(int argc, char *argv[])
{
	unsigned io_addr = 0x1800;
	unsigned wIndex = 1;
	unsigned status;
	int ofs;

	iopl(3);

#if 0
	outw(USBCMD_EGSM, io_addr + USBCMD);
	outw(USBCMD_EGSM, io_addr + USBCMD + 0x20);
	outw(USBCMD_EGSM, io_addr + USBCMD + 0x40);
#endif

#if 0
	outw(USBCMD_GRESET, io_addr + USBCMD);
	outw(USBCMD_GRESET, io_addr + USBCMD + 0x20);
	outw(USBCMD_GRESET, io_addr + USBCMD + 0x40);
#endif

	for (wIndex=1;wIndex<100;wIndex++) {
	for (ofs=0;ofs <= 0x40; ofs += 0x20) {
		status = inw(io_addr + USBPORTSC1 + 2 * (wIndex-1));
		status = (status & 0xfff5) & ~RH_PORT_POWER;
		outw(status, io_addr + USBPORTSC1 + 2 * (wIndex-1));
	}
	}

	exit(0);
}
