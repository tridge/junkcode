CFLAGS = -Wall -DWITH_SENDFILE=1

all: keepalive

keepalive: keepalive.o
	$(CC) -o $@ keepalive.o

clean:
	rm -f *.o keepalive *~

