CC=E:\VC98\bin\cl -nologo
INCLUDES=-I 
CFLAGS=$(INCLUDES) -Zi
LIBS=

all: writefiles

clean:
	del *~ *.obj writefiles

writefiles: writefiles.obj
	$(CC) -o writefiles writefiles.obj $(LIBS)

writefiles.obj: writefiles.h writefiles.c
