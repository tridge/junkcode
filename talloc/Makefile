#OPT = -g -ftest-coverage -fprofile-arcs 
OPT = -O2
CFLAGS = $(OPT) -Wall -D_STANDALONE_

testsuite: talloc.o testsuite.o
	$(CC) $(CFLAGS) -o testsuite testsuite.o talloc.o

clean:
	rm -f *~ *.o testsuite *.gc??

