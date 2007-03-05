CC=gcc
CFLAGS=-fPIC -Wall -g

preload_softroot.so: preload_softroot.o
	ld -shared -o preload_softroot.so preload_softroot.o -g -ldl

clean:
	rm -f preload_softroot.so preload_softroot.o
