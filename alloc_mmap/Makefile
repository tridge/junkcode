CFLAGS=-Wall -O2

all: alloc_mmap.so

alloc_mmap.so: alloc_mmap.o
	ld -shared -o $@ $^ $(LIBS)

clean:
	/bin/rm -f *.o *.so *~
