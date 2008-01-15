CFLAGS=-Wall -O3 

.SUFFIXES: .po .o .c .so .to

.c.po:
	$(CC) $(CFLAGS) -c -fPIC -o $@ $^

.c.to:
	$(CC) $(CFLAGS) -DTHREAD_SAFE -c -fPIC -o $@ $^

all: alloc_mmap.so alloc_mmap_thread_safe.so testsuite testsuite_malloc testsuite_thread_safe

alloc_mmap.so: alloc_mmap.po
	ld -shared -o $@ $^ $(LIBS)

alloc_mmap_thread_safe.so: alloc_mmap.to
	ld -shared -o $@ $^ $(LIBS) -lpthread

testsuite: alloc_mmap.o testsuite.o
	$(CC) $(CFLAGS) -o testsuite alloc_mmap.o testsuite.o

testsuite_malloc: testsuite.o
	$(CC) $(CFLAGS) -o testsuite_malloc testsuite.o

testsuite_pthread: testsuite.o
	$(CC) $(CFLAGS) -o testsuite_pthread testsuite.o -lpthread

testsuite_thread_safe: testsuite.o alloc_mmap.to
	$(CC) $(CFLAGS) -o testsuite_thread_safe alloc_mmap.to testsuite.o -lpthread

clean:
	/bin/rm -f *.o *.so *.to *.po *~ *.gc?? testsuite_thread_safe testsuite_malloc testsuite_pthread testsuite


