CFLAGS=-Wall -g -DSTANDALONE=1
CC=gcc

all: test

parse_info.h: test.h
	./genstruct.pl --header=test.h --cfile=parse_info.h

test: parse_info.h test.o genparser.o 
	$(CC) $(CFLAGS) -o test test.o genparser.o

clean:
	rm -f *.o test parse_info.h *~
