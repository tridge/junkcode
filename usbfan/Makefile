CFLAGS = -Wall -O2
OBJ = usbfan.o pci.o

all: usbfan

usbfan: $(OBJ)
	$(CC) -o usbfan $(OBJ)

clean:
	rm -f $(OBJ) usbfan *~
