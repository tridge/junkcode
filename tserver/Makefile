CC=gcc
CFLAGS=-O6 -Wall -Wshadow -Wstrict-prototypes -Wpointer-arith -Wcast-qual -Wcast-align
CPPFLAGS=-D_GNU_SOURCE=1
OBJS = util.o tserver.o cgi.o template.o
LIBS =
INCLUDES = includes.h cgi.h template.h

%.o: %.c $(INCLUDES)
	@echo Compiling $*.c
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) $*.c 

all: tserver template

tserver: $(OBJS)
	@echo Linking $@
	@$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)
	@strip $@

template: template.o template_main.o util.o cgi.o
	@echo Linking $@
	@$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	/bin/rm -f *.o tserver template *~ files/*~ 

