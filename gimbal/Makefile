CFLAGS = -Wall -O2 -DMAIN_TEST
CC = gcc
LIBS = -lm

all:
	$(CC) $(CFLAGS) -o gimbal_attitude gimbal_attitude.c $(LIBS)

test: all
	@python3 gimbal_attitude.py
	@./gimbal_attitude

