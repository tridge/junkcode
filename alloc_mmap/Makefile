CFLAGS=-Wall -fPIC -O2
#CFLAGS=-Wall -fPIC -ftest-coverage -fprofile-arcs 

all: alloc_mmap.so testsuite testsuite_malloc

alloc_mmap.so: alloc_mmap.o
	ld -shared -o $@ $^ $(LIBS)

testsuite: alloc_mmap.o testsuite.o
	$(CC) $(CFLAGS) -o testsuite alloc_mmap.o testsuite.o

testsuite_malloc: testsuite.o
	$(CC) $(CFLAGS) -o testsuite_malloc testsuite.o

clean:
	/bin/rm -f *.o *.so *~ *.gc??

