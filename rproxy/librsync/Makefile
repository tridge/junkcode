CFLAGS=-I. -g 
LDFLAGS=-lrsync -L.

LIBOBJS= hash.o checksum.o librsync-funcs.o 

librsync.so: $(LIBOBJS)
	$(LD) -shared -o $@ $(LIBOBJS)

LIBRSYNCUTILS=librsync-signature librsync-encode librsync-decode testlibrsync

all: librsync.so librsync-utils

clean:
	rm -f *.o librsync.so $(LIBRSYNCUTILS) *~


testlibrsync: testlibrsync.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

librsync-signature: librsync-signature.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

librsync-encode: librsync-encode.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

librsync-decode: librsync-decode.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

librsync-utils: $(LIBRSYNCUTILS)

test: librsync librsync-utils

