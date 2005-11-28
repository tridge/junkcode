CFLAGS=-Wall -fPIC -O2
LIBS=-lm

all: load_math.so 

load_math.so: load_math.o
	ld -shared -o $@ $^ $(LIBS)

clean:
	/bin/rm -f *.o *.so *~

