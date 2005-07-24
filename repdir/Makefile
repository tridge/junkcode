CC=gcc
CFLAGS=-Wall -g

all: os2_delete bsd_telldir os2_delete_broken bsd_telldir_broken

os2_delete: os2_delete.o repdir.o
	$(CC) -o os2_delete os2_delete.o repdir.o

bsd_telldir: bsd_telldir.o repdir.o
	$(CC) -o bsd_telldir bsd_telldir.o repdir.o

os2_delete_broken: os2_delete.o
	$(CC) -o os2_delete_broken os2_delete.o

bsd_telldir_broken: bsd_telldir.o
	$(CC) -o bsd_telldir_broken bsd_telldir.o

clean:
	rm -f *.o os2_delete bsd_telldir *~
