
forkgdb.so: forkgdb.c
	gcc -Wall -c -fPIC forkgdb.c
	ld -shared -o forkgdb.so forkgdb.o -ldl

clean:
	rm -f *.o *.so
