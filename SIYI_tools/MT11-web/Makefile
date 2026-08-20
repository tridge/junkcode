CROSS_COMPILE ?= aarch64-linux-gnu-
CC := $(CROSS_COMPILE)gcc
CFLAGS ?= -O2 -pipe -Wall -Wextra -Werror -std=c11
LDFLAGS ?= -static

.PHONY: all clean

all: mt11-web

mt11-web: mt11-web.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f mt11-web
