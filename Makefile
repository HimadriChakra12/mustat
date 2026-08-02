CC      = gcc
CFLAGS  = -O2 -Wall -std=c99 -D_POSIX_C_SOURCE=200809L \
          -I/usr/include/freetype2 -I/usr/include/libpng16
LIBS    = -lXft -lX11
PREFIX  = /usr/local
BINDIR  = $(PREFIX)/bin

HOMEDIR ?= $(shell getent passwd $$(logname 2>/dev/null || echo $(SUDO_USER)) | cut -d: -f6)
HOMEDIR ?= $(shell getent passwd $$(logname) | cut -d: -f6)

LUA_CFLAGS := $(shell pkg-config --cflags lua5.4 2>/dev/null || pkg-config --cflags lua-5.4 2>/dev/null || pkg-config --cflags lua 2>/dev/null)
LUA_LIBS   := $(shell pkg-config --libs   lua5.4 2>/dev/null || pkg-config --libs   lua-5.4 2>/dev/null || pkg-config --libs   lua 2>/dev/null)

all: mustat mublocks/mustat-blocks

mustat: main.c bar.c draw.c workspace.c ws_render.c tray.c
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

mublocks/mustat-blocks: mublocks/main.c mublocks/module.c mublocks/lua_config.c
	$(CC) $(CFLAGS) $(LUA_CFLAGS) -Imublocks $^ -o $@ $(LUA_LIBS)

clean:
	rm -f mustat mublocks/mustat-blocks

install:
	install mustat $(PREFIX)/bin/mustat
	install mublocks/mustat-blocks $(PREFIX)/bin/mustat-blocks
	@mkdir -p $(HOMEDIR)/.config/mustat
	install data/mu.lua $(HOMEDIR)/.config/mustat/mu.lua

.PHONY: all clean
