CC=gcc
CFLAGS=-Wall -g -I$(BUILD_BASE)/8051/kernel
OBJS=util.o

all: ux_server ux_client

ux_server: ux_server.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

ux_client: ux_client.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o *~ ux_client ux_server
