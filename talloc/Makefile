CFLAGS = -O2 -Wall -D_STANDALONE_

testsuite: talloc.o testsuite.o
	$(CC) -o testsuite testsuite.o talloc.o

clean:
	rm -f *~ *.o testsuite
