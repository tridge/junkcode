CFLAGS = -Wall

all: sock_sink sock_source

sock_sink: sock_sink.o socklib.o
	$(CC) -o $@ sock_sink.o socklib.o

sock_source: sock_source.o socklib.o
	$(CC) -o $@ sock_source.o socklib.o

clean:
	rm -f *.o sock_sink sock_source *~
