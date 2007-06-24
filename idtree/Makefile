CFLAGS=-Wall -O2 -I/usr/src/linux/include

all: idtree

idtree: idtree.o idtree.h
	$(CC) $(CFLAGS) -o idtree idtree.o

clean:
	rm -f *.o *~ idtree
