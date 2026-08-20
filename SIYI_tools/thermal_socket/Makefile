CFLAGS = -static -Wall -Wextra -Werror -O2 -g3

# The MT11 is AArch64. ZLIB_LIB must be an AArch64 static zlib archive.
CC = aarch64-linux-gnu-gcc
ZLIB_LIB = libz-aarch64.a
LIBS = $(ZLIB_LIB)

all: thermal_socket

thermal_socket: thermal_socket.c
	$(CC) $(CFLAGS) -o thermal_socket thermal_socket.c $(LIBS)

clean:
	/bin/rm -f  thermal_socket
