CC=gcc
CFLAGS=-Wall -Iinclude/mavlink -g

mavexample: mavhelper.c main.c
	$(CC) $(CFLAGS) -o mavexample mavhelper.c main.c

mavgen:
	rm -rf include/mavlink
	mkdir -p include/mavlink
	mavgen.py --lang C -o include/mavlink mavlink/message_definitions/v1.0/ardupilotmega.xml

clean:
	rm -f mavexample

