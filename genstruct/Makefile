CFLAGS=-Wall -DSTANDALONE=1 -O2 -g
CC=gcc

all: test

parse_info.h: test.h
	./genstruct.pl -o parse_info.h -- $(CC) -E $(CFLAGS) test.c

test: parse_info.h test.o genparser.o 
	$(CC) $(CFLAGS) -o test test.o genparser.o

clean:
	rm -f *.o test parse_info.h *~
