CFLAGS=-DHAVE_MMAP -DSTANDALONE -Wall -D__USE_GNU -O2

SOURCE=mdfour.c tsums.c tdb.c spinlock.c

OBJ=$(SOURCE:.c=.o)

tsums: $(OBJ)
	$(CC) -o tsums $(OBJ)

clean:
	/bin/rm -f *.o *~ tsums
