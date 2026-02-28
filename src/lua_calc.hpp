#pragma once
#include <sdk/os/lcd.h>
#include "lua/lua.hpp"

extern "C" {
int luaopen_calc(lua_State *L);
}
