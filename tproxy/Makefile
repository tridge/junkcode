CC=gcc
CFLAGS=-g -Wall -Wshadow -Wstrict-prototypes -Wpointer-arith -Wcast-qual -Wcast-align
CPPFLAGS=-D_GNU_SOURCE=1
OBJS = util.o tproxy.o cgi.o 
LIBS =
INCLUDES = includes.h cgi.h template.h

%.o: %.c $(INCLUDES)
	@echo Compiling $*.c
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) $*.c 

all: tproxy

tproxy: $(OBJS)
	@echo Linking $@
	@$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	/bin/rm -f *.o tproxy *~

