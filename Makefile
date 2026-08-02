CC      = clang
CFLAGS  = -O2 -Wall -std=c99 -D_POSIX_C_SOURCE=200809L \
          -I/usr/include/freetype2 -I/usr/include/libpng16
LIBS    = -lXft -lX11
PREFIX  = /usr/local
BINDIR  = $(PREFIX)/bin

HOMEDIR ?= $(shell getent passwd $$(logname 2>/dev/null || echo $(SUDO_USER)) | cut -d: -f6)
OWNER   := $(shell logname 2>/dev/null || echo $(SUDO_USER))

LUA_CFLAGS := $(shell pkg-config --cflags lua5.4 2>/dev/null || pkg-config --cflags lua-5.4 2>/dev/null || pkg-config --cflags lua 2>/dev/null)
LUA_LIBS   := $(shell pkg-config --libs   lua5.4 2>/dev/null || pkg-config --libs   lua-5.4 2>/dev/null || pkg-config --libs   lua 2>/dev/null)

all: mustat mublocks/mustat-blocks

# config.h is your local, editable copy — created once from the
# tracked defaults and never touched again, even if config.def.h
# changes later. Delete config.h yourself if you want to reset it.
config.h:
	cp config.def.h $@

mustat: config.h main.c bar.c draw.c workspace.c ws_render.c tray.c
	$(CC) $(CFLAGS) main.c bar.c draw.c workspace.c ws_render.c tray.c -o $@ $(LIBS)

mublocks/mustat-blocks: mublocks/main.c mublocks/module.c mublocks/lua_config.c
	$(CC) $(CFLAGS) $(LUA_CFLAGS) -Imublocks $^ -o $@ $(LUA_LIBS)

clean:
	rm -f mustat mublocks/mustat-blocks

install:
	install mustat $(PREFIX)/bin/mustat
	install mublocks/mustat-blocks $(PREFIX)/bin/mustat-blocks
	install -d -o $(OWNER) -g $(OWNER) $(HOMEDIR)/.config/mustat
	@if [ -e $(HOMEDIR)/.config/mustat/mu.lua ]; then \
		echo "install: $(HOMEDIR)/.config/mustat/mu.lua already exists, leaving it alone"; \
	else \
		install -o $(OWNER) -g $(OWNER) data/mu.lua $(HOMEDIR)/.config/mustat/mu.lua; \
	fi

.PHONY: all clean
