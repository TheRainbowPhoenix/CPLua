#include <sdk/calc/calc.h>
#include "cplua.hpp"
#include "lua_calc.hpp"
#include <sdk/os/debug.h>
#include <sdk/os/lcd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <type_traits>

static int debug_y = 0;
#ifdef DEBUG
#define TRACE(msg) Debug_Printf(0, debug_y++, false, 0, msg); LCD_Refresh()
#else
#define TRACE(msg)
#endif


extern "C" {
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
}

// A backup pointer for the original vram content
std::remove_pointer_t<decltype(vram)> (*vram_bak)[width * height];

// Simple RAII file handle wrapper
class SafeFileHandle {
    FILE* fp;
public:
    SafeFileHandle(const char* filepath) {
        fp = fopen(filepath, "rb");
    }
    ~SafeFileHandle() {
        if (fp) fclose(fp);
    }
    bool is_valid() const { return fp != nullptr; }
    FILE* get() const { return fp; }
};


extern "C" {
    void cplua_print(const char* s, int l) {
#ifdef DEBUG
        Debug_Printf(0, debug_y++, false, 0, "LUA PRINT: %.*s", l, s);
        LCD_Refresh();
#else
        (void)s;
        (void)l;
#endif
    }
    void cplua_println() {
#ifdef DEBUG
        Debug_Printf(0, debug_y++, false, 0, "LUA PRINTLN");
        LCD_Refresh();
#endif
    }
}

bool RunLuaScript(const std::string& filepath) {
    // 1. Setup backup screen
    vram_bak = (decltype(vram_bak))malloc(sizeof(*vram_bak));
    if (!vram_bak) { return false; }

    memcpy(vram_bak, vram, sizeof(*vram_bak));

#ifdef DEBUG
    debug_y = 0;
    LCD_ClearScreen();
    TRACE("Starting RunLuaScript");
#endif
    LCD_ClearScreen();
    TRACE("VRAM backed up");

    // 2. Init Lua
    lua_State *L = luaL_newstate();
    if (!L) {
        TRACE("luaL_newstate failed");
        free(vram_bak);
        return false;
    }
    TRACE("Lua state created");

    luaL_openlibs(L);
    luaL_requiref(L, "calc", luaopen_calc, 1);
    lua_pop(L, 1);  // remove lib
    TRACE("Lua libs opened");

    // 3. Open and Read File using SDK functions to avoid crashing on calc
    SafeFileHandle file(filepath.c_str());
    if (!file.is_valid()) {
        TRACE("File open failed");
        lua_close(L);
        memcpy(vram, vram_bak, sizeof(*vram_bak));
        LCD_Refresh();
        free(vram_bak);
        return false;
    }
    TRACE("File opened");

    // Use seek to get file size instead of fstat to avoid POSIX issues with CP handles
    fseek(file.get(), 0, SEEK_END);
    int file_size = ftell(file.get());
    fseek(file.get(), 0, SEEK_SET);

    if (file_size <= 0) {
        TRACE("File size <= 0");
        lua_close(L);
        memcpy(vram, vram_bak, sizeof(*vram_bak));
        LCD_Refresh();
        free(vram_bak);
        return false;
    }
    TRACE("File size determined");

    char* scriptContent = (char*)malloc(file_size + 1);
    if (!scriptContent) {
        TRACE("Script malloc failed");
        lua_close(L);
        memcpy(vram, vram_bak, sizeof(*vram_bak));
        LCD_Refresh();
        free(vram_bak);
        return false;
    }

    fread(scriptContent, 1, file_size, file.get());
    scriptContent[file_size] = '\0';
    TRACE("File read");

    // 4. Run Script
    TRACE("Calling luaL_dostring...");
    if (luaL_dostring(L, scriptContent)) {
        // Output error to screen
        const char* err = lua_tostring(L, -1);
#ifdef DEBUG
        Debug_Printf(0, debug_y++, false, 0, "LUA ERR: %s", err);
        LCD_Refresh();
#endif
    }
    TRACE("luaL_dostring finished");

    // Wait for user to dismiss
#ifdef DEBUG
    Debug_Printf(0, debug_y++, false, 0, "Press any key to exit...");
    LCD_Refresh();
#endif
    Debug_WaitKey();

    // 5. Cleanup
    TRACE("Cleaning up...");
    free(scriptContent);
    lua_close(L);

    // Restore Screen
    memcpy(vram, vram_bak, sizeof(*vram_bak));
    LCD_Refresh();
    free(vram_bak);

    return true;
}