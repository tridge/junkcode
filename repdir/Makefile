CC=gcc
CFLAGS=-Wall -g

all: os2_delete bsd_telldir

os2_delete: os2_delete.o repdir.o
	$(CC) -o os2_delete os2_delete.o repdir.o

bsd_telldir: bsd_telldir.o repdir.o
	$(CC) -o bsd_telldir bsd_telldir.o repdir.o

clean:
	rm -f *.o os2_delete bsd_telldir *~
