CFLAGS=-fpic -O2 -Wall

all: fakecd.so fix_recording.so

fakecd.so: fakecd.o
	ld -shared -o fakecd.so fakecd.o -ldl

fix_recording.so: fix_recording.o
	ld -shared -o fix_recording.so fix_recording.o -ldl

clean:
	/bin/rm -f *.o *.so *~
