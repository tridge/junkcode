CC=cl -nologo
INCLUDES=-I 
CFLAGS=$(INCLUDES) -Zi
LIBS=

all: writefiles

clean:
	del *~ *.obj writefiles.exe

writefiles: writefiles.obj
	$(CC) /Fewritefiles.exe writefiles.obj $(LIBS)

writefiles.obj: writefiles.h writefiles.c
