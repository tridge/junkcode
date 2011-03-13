CFLAGS=-Wall -g -Wshadow -Wstrict-prototypes -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings -Werror-implicit-function-declaration -Wformat=2 -Wno-format-y2k -Wmissing-prototypes -Wconversion 
GCCBASE=/usr/lib/gcc/x86_64-linux-gnu/4.4.5

all: nttime_test.o sparse_test.o

clean:
	rm -f *.o *~

.c.o:
	sparse -Wbitwise -gcc-base-dir $(GCCBASE) $<
	gcc $(CFLAGS) -c $<
