CC=gcc
CFLAGS=-O6 -Wall -Wshadow -Wstrict-prototypes -Wpointer-arith -Wcast-qual -Wcast-align
CPPFLAGS=-D_GNU_SOURCE=1
OBJS = util.o tserver.o cgi.o template.o
LIBS =
INCLUDES = includes.h cgi.h template.h

%.o: %.c $(INCLUDES)
	@echo Compiling $*.c
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) $*.c 

tserver: $(OBJS)
	@echo Linking $@
	@$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)
	@strip $@

clean:
	/bin/rm -f *.o tserver *~
