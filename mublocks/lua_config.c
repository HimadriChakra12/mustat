#define _POSIX_C_SOURCE 200809L

#include "lua_config.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

static Block *g_blocks = NULL;
static int    g_count  = 0;
static int    g_cap    = 0;

static int name_taken(const char *name)
{
    for (int i = 0; i < g_count; i++)
        if (strcmp(g_blocks[i].name, name) == 0) return 1;
    return 0;
}

static void push_block(const Block *b)
{
    if (g_count >= g_cap) {
        g_cap = g_cap ? g_cap * 2 : 8;
        g_blocks = realloc(g_blocks, g_cap * sizeof(Block));
    }
    g_blocks[g_count++] = *b;
}

/* module { name="clock", type="clock", cmd="date +%H:%M", pos="RIGHT",
 *          interval=1, enabled=true }
 *
 * `name` is the block's identity — this is what mustat uses to tell
 * blocks apart, so it must be unique across the whole config (not
 * just within one `pos`). Two modules can share the same `pos`
 * freely; they just need different names.
 *
 * `type` picks a fast built-in (clock/cpu/mem); anything else (or
 * omitted) falls back to running `cmd` as a shell command every
 * `interval` seconds — this is the dynamic-variable path, since
 * $(...) shell substitutions inside cmd are re-evaluated fresh on
 * every popen() call. */
static int l_module(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);

    Block b = {0};
    b.enabled  = 1;
    b.interval = 1;
    snprintf(b.pos, sizeof(b.pos), "RIGHT");

    lua_getfield(L, 1, "name");
    if (lua_isstring(L, -1)) snprintf(b.name, sizeof(b.name), "%s", lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, 1, "type");
    const char *ty = lua_isstring(L, -1) ? lua_tostring(L, -1) : "script";
    if      (!strcmp(ty, "clock")) b.type = MOD_CLOCK;
    else if (!strcmp(ty, "cpu"))   b.type = MOD_CPU;
    else if (!strcmp(ty, "mem"))   b.type = MOD_MEM;
    else                           b.type = MOD_SCRIPT;
    lua_pop(L, 1);

    lua_getfield(L, 1, "cmd");
    if (lua_isstring(L, -1)) snprintf(b.cmd, sizeof(b.cmd), "%s", lua_tostring(L, -1));
    lua_pop(L, 1);

    lua_getfield(L, 1, "pos");
    if (lua_isstring(L, -1)) {
        char up[8]; int i = 0;
        const char *p = lua_tostring(L, -1);
        for (; p[i] && i < 7; i++) up[i] = toupper((unsigned char)p[i]);
        up[i] = 0;
        snprintf(b.pos, sizeof(b.pos), "%s", up);
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "interval");
    if (lua_isnumber(L, -1)) b.interval = (int)lua_tonumber(L, -1);
    if (b.interval < 1) b.interval = 1;
    lua_pop(L, 1);

    lua_getfield(L, 1, "enabled");
    if (lua_isboolean(L, -1)) b.enabled = lua_toboolean(L, -1);
    lua_pop(L, 1);

    if (!b.enabled) return 0;
    if (b.type == MOD_SCRIPT && !b.cmd[0]) return 0;

    if (!b.name[0]) {
        fprintf(stderr, "mustat-blocks: module missing `name`, skipping (cmd=\"%s\")\n", b.cmd);
        return 0;
    }
    if (name_taken(b.name)) {
        fprintf(stderr,
            "mustat-blocks: two modules named \"%s\" — names must be unique "
            "(that's how mustat tells blocks apart), skipping the later one\n",
            b.name);
        return 0;
    }

    push_block(&b);
    return 0;
}

static int find_config(char *path, size_t size)
{
    const char *xdg  = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");

    if (xdg) {
        snprintf(path, size, "%s/mustat/mu.lua", xdg);
        if (access(path, R_OK) == 0) return 1;
    }
    if (home) {
        snprintf(path, size, "%s/.config/mustat/mu.lua", home);
        if (access(path, R_OK) == 0) return 1;
    }
    snprintf(path, size, "/etc/mustat/mu.lua");
    if (access(path, R_OK) == 0) return 1;

    return 0;
}

int lua_config_load(Block **out)
{
    g_blocks = NULL;
    g_count  = 0;
    g_cap    = 0;

    char path[512];
    if (!find_config(path, sizeof(path))) {
        *out = NULL;
        return 0;
    }

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    lua_pushcfunction(L, l_module);
    lua_setglobal(L, "module");

    if (luaL_dofile(L, path) != LUA_OK) {
        fprintf(stderr, "mustat-blocks: %s\n", lua_tostring(L, -1));
    }

    lua_close(L);
    *out = g_blocks;
    return g_count;
}
