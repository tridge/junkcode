CC=gcc
CFLAGS=-Wall -Iinclude/mavlink -g

ntrip_mavlink: ntrip_mavlink.c main.c
	$(CC) $(CFLAGS) -o ntrip_mavlink ntrip_mavlink.c main.c

clean:
	rm -f ntrip_mavlink
