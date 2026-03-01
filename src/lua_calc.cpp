#include <sdk/calc/calc.h>
#include "lua_calc.hpp"

// calc.clear()
static int l_clear(lua_State *L) {
    (void)L;
    LCD_ClearScreen();
    return 0;
}

// calc.refresh()
static int l_refresh(lua_State *L) {
    (void)L;
    LCD_Refresh();
    return 0;
}

// calc.set_pixel(x, y, color)
static int l_set_pixel(lua_State *L) {
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int color = luaL_checkinteger(L, 3);

    // Safety check
    if (x >= 0 && (unsigned int)x < width && y >= 0 && (unsigned int)y < height) {
        vram[y * width + x] = static_cast<uint16_t>(color);
    }

    return 0;
}

static const struct luaL_Reg calc_lib[] = {
    {"clear", l_clear},
    {"refresh", l_refresh},
    {"set_pixel", l_set_pixel},
    {NULL, NULL}
};

extern "C" {
int luaopen_calc(lua_State *L) {
    luaL_newlib(L, calc_lib);
    return 1;
}
}
