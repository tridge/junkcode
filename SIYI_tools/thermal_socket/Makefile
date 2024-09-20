CFLAGS = -static -Wall -Wextra -g3

CC=arm-linux-gnueabihf-gcc
#CC=gcc
LIBS = libz.a

all: thermal_socket

thermal_socket: thermal_socket.c
	$(CC) $(CFLAGS) -o thermal_socket thermal_socket.c $(LIBS)

clean:
	/bin/rm -f  thermal_socket
