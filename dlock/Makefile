CFLAGS=-Wall -O2

LIBS=

SERVER_OBJ=server.o process.o util.o 
CLIENT_OBJ=dl_client.o util.o dl_ping_pong.o

all: dl_server dl_ping_pong

dl_server: $(SERVER_OBJ)
	$(CC) -o dl_server $(SERVER_OBJ) $(LIBS)

dl_ping_pong: $(CLIENT_OBJ)
	$(CC) -o dl_ping_pong $(CLIENT_OBJ) $(LIBS)

clean:
	/bin/rm -f *.o *~ dl_server dl_ping_pong
