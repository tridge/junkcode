CFLAGS=-fpic -O2 -Wall

depmaker.so: depmaker.o
	ld -shared -o depmaker.so depmaker.o -ldl

clean:
	/bin/rm -f depmaker.o depmaker.so *~
