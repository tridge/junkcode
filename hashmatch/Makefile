CFLAGS=-Wall -g

OBJ = common.o md4.o blockhash.o hashmatch.o hashmatch2.o

all: blockhash hashmatch hashmatch2

blockhash: $(OBJ)
	$(CC) $(CFLAGS) -o blockhash blockhash.o common.o md4.o

hashmatch: $(OBJ)
	$(CC) $(CFLAGS) -o hashmatch hashmatch.o common.o md4.o

hashmatch2: $(OBJ)
	$(CC) $(CFLAGS) -o hashmatch2 hashmatch2.o common.o md4.o

clean:
	rm -f *.o *~ blockhash hashmatch hashmatch2


