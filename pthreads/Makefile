CFLAGS=-Wall
LIBS=-lpthread

all: tls euid locking

tls: tls.o
	$(CC) -o tls tls.o $(LIBS)

euid: euid.o
	$(CC) -o euid euid.o $(LIBS)

locking: locking.o
	$(CC) -o locking locking.o $(LIBS)

clean:
	rm -f *~ euid tls *.o
