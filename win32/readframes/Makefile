CC=E:\VC98\bin\cl -nologo
INCLUDES=-I 
CFLAGS=$(INCLUDES) -Zi
LIBS=

all: readframes

clean:
	del *~ *.obj readframes

readframes: readframes.obj
	$(CC) -o readframes readframes.obj $(LIBS)

readframes.obj: readframes.h readframes.c
