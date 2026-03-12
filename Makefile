CC      = gcc
CFLAGS  = -O2 -Wall -std=c99 -D_POSIX_C_SOURCE=200809L \
          -I/usr/include/freetype2 -I/usr/include/libpng16
LIBS    = -lXft -lX11

all: ibar iblocks/iblocks

ibar: main.c bar.c draw.c i3ipc.c
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

iblocks/iblocks: iblocks/main.c iblocks/module.c
	$(CC) $(CFLAGS) -Iiblocks $^ -o $@

clean:
	rm -f ibar iblocks/iblocks
