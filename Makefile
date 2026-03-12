CC=gcc
CFLAGS=-O2 -Wall -std=c99 -D_POSIX_C_SOURCE=200809L $(shell pkg-config --cflags xft)
LIBS=$(shell pkg-config --libs xft) -lX11 -ljson-c

SRC=main.c bar.c draw.c module.c i3ipc.c

ibar:
	$(CC) $(CFLAGS) $(SRC) -o ibar $(LIBS)

clean:
	rm -f ibar
