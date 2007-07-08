CFLAGS=-Wall -O3

.SUFFIXES: .po .o .c .so

.c.po:
	$(CC) $(CFLAGS) -c -fPIC -o $@ $^

all: alloc_mmap.so testsuite testsuite_malloc

alloc_mmap.so: alloc_mmap.po
	ld -shared -o $@ $^ $(LIBS)

testsuite: alloc_mmap.o testsuite.o
	$(CC) $(CFLAGS) -o testsuite alloc_mmap.o testsuite.o

testsuite_malloc: testsuite.o
	$(CC) $(CFLAGS) -o testsuite_malloc testsuite.o

testsuite_pthread: testsuite.o
	$(CC) $(CFLAGS) -o testsuite_pthread testsuite.o -lpthread

clean:
	/bin/rm -f *.o *.so *.po *~ *.gc??
