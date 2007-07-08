
CFLAGS = -Wall -O3

all: keysearch

OBJS=md5.o lsakey.o hmacmd5.o arcfour.o des.o

lsakey: $(OBJS)
	$(CC) -o lsakey $(OBJS)

keysearch: des.o keysearch.o
	$(CC) -o keysearch des.o keysearch.o

clean:
	rm -f *.o *~ lsakey keysearch

