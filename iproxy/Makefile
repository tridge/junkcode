CFLAGS=-Wall -g
CC=gcc

PROGS=iproxy-server iproxy

all: $(PROGS)

iproxy-server: util.o iproxy-server.o
	$(CC) -o $@ $^

iproxy: util.o iproxy.o
	$(CC) -o $@ $^

clean:
	/bin/rm -f *.o *~ $(PROGS)


