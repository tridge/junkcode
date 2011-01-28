all: preload_usb.so set_lcd

CC=gcc
CFLAGS=-Wall

preload_usb.so: preload_usb.c
	gcc -Wall -fPIC -g -shared -o preload_usb.so preload_usb.c -ldl

clean:
	rm -f *~ *.so set_lcd

