# Change this to point to your Linux kernel sources
LINUX_SRC=	/usr/src/linux

# Uncomment this line to build the module with MODVERSIONS if your
# kernel was built that way (or else depmod -a will scream bloody
# murder every time it gets run at boot time)
# MODVERSIONS= -DMODVERSIONS

CFLAGS=-D__KERNEL__ -I$(LINUX_SRC)/include -Wall -Wstrict-prototypes \
	-Wno-trigraphs -O2 -fomit-frame-pointer -fno-strict-aliasing \
	-march=i686 -DMODULE $(MODVERSIONS) \
	-include $(LINUX_SRC)/include/linux/modversions.h

all: trd.o dummy_disk.o

install:
	cp trd.o /lib/modules/`uname -r`/kernel/drivers/block

clean:
	/bin/rm -f *.o *~
