CC = gcc
CFLAGS = -Wall -W -O2

OBJS = spamsum.o edit_dist.o

spamsum: $(OBJS)
	$(CC) -o spamsum $(OBJS)

clean:
	rm -f $(OBJS) spamsum *~
