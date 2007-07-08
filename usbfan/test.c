#include "usbfan.h"

main()
{
	int i;
	iopl(3);
	for (i=0;i<10000;i++) {
		int port = 0x1800 + (random() % 96);
		if (random() % 2 == 0) {
			outb(random() % 256, port);
		} else {
			inb(port);
		}
	}
}
