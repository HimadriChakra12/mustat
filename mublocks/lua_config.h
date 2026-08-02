#ifndef LUA_CONFIG_H
#define LUA_CONFIG_H

#include "module.h"

/* Looks for mu.lua in, in order:
 *   $XDG_CONFIG_HOME/mustat/mu.lua
 *   $HOME/.config/mustat/mu.lua
 *   /etc/mustat/mu.lua
 * Runs it and collects every module{...} call into *out (malloc'd,
 * caller must free()). Returns the block count, 0 if no config file
 * was found or it registered nothing. */
int lua_config_load(Block **out);

#endif
