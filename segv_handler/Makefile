CFLAGS=-Wall -fPIC -O2

all: segv_handler.so testprog

segv_handler.so: segv_handler.o
	ld -shared -o $@ $^ $(LIBS)

testprog: testprog.c
	$(CC) -g -o testprog{,.c}

clean:
	/bin/rm -f *.o *.so *~ testprog

