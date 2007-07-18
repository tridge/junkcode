CFLAGS=-Wall -fPIC -O2
LIBS=-ldl

all: mkdir_preload.so

mkdir_preload.so: mkdir_preload.o
	ld -shared -o $@ $^ $(LIBS)

clean:
	/bin/rm -f *.o *.so *~ 

