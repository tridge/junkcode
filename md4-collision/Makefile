CFLAGS=-Wall -O3

all: md4-collision

md4-collision: md4-collision.o mdfour.o
	$(CC) -o md4-collision md4-collision.o mdfour.o

clean:
	rm -f *.o *~ md4-collision

