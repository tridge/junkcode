CFLAGS=-Wall

# for linux
CC=gcc
LIBS=

# for solaris
#CC=cc
#LIBS=-lsocket -lnsl

all: userver uclient

userver: userver.o
	$(CC) -o userver userver.o $(LIBS)

uclient: uclient.o
	$(CC) -o uclient uclient.o $(LIBS)

clean:
	/bin/rm -f *.o *~ userver uclient
