CFLAGS=-Wall -O2 -D_GNU_SOURCE=1
OBJS = util.o tserver.o cgi.o

tserver: $(OBJS)
	$(CC) $(CFLAGS) -o tserver $(OBJS)
	strip tserver

clean:
	/bin/rm -f *.o tserver *~
