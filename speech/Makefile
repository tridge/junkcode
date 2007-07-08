LFLAGS = -lm -O -s
CFLAGS = -O2
CC=gcc

manipulate : manipulate.o speech.h util.o myfft.o cepstral.o
	gcc -o manipulate manipulate.o util.o myfft.o cepstral.o $(LFLAGS)

quantise : quantise.o
	gcc -O -o ../bin/quantise quantise.c 
