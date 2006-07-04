CC=gcc
CFLAGS=-fPIC -Wall -g

preload_fileops.so: preload_fileops.o
	ld -shared -o preload_fileops.so preload_fileops.o -g -ldl

clean:
	rm -f preload_fileops.so preload_fileops.o
