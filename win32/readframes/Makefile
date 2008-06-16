CC=cl -nologo
INCLUDES=-I 
CFLAGS=$(INCLUDES) -Zi
LIBS=

all: readframes

clean:
	del *~ *.obj readframes.exe

readframes: readframes.obj
	$(CC) /Fereadframes.exe readframes.obj $(LIBS)

readframes.obj: readframes.h readframes.c
