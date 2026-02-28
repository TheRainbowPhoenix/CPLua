#include <sdk/calc/calc.h>
#include "cplua.hpp"
#include "lua_calc.hpp"
#include <sdk/os/debug.h>
#include <sdk/os/lcd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <type_traits>

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
        Debug_Printf(0, 0, false, 0, "%.*s", l, s);
    }
    void cplua_println() {
        Debug_Printf(0, 0, false, 0, "\n");
    }
}

bool RunLuaScript(const std::string& filepath) {
    // 1. Setup backup screen
    vram_bak = (decltype(vram_bak))malloc(sizeof(*vram_bak));
    if (!vram_bak) return false;

    memcpy(vram_bak, vram, sizeof(*vram_bak));
    LCD_ClearScreen();

    // 2. Init Lua
    lua_State *L = luaL_newstate();
    if (!L) {
        free(vram_bak);
        return false;
    }

    luaL_openlibs(L);
    luaL_requiref(L, "calc", luaopen_calc, 1);
    lua_pop(L, 1);  // remove lib

    // 3. Open and Read File using SDK functions to avoid crashing on calc
    SafeFileHandle file(filepath.c_str());
    if (!file.is_valid()) {
        lua_close(L);
        free(vram_bak);
        return false;
    }

    // Use seek to get file size instead of fstat to avoid POSIX issues with CP handles
    fseek(file.get(), 0, SEEK_END);
    int file_size = ftell(file.get());
    fseek(file.get(), 0, SEEK_SET);

    if (file_size <= 0) {
        lua_close(L);
        memcpy(vram, vram_bak, sizeof(*vram_bak));
        LCD_Refresh();
        free(vram_bak);
        return false;
    }

    char* scriptContent = (char*)malloc(file_size + 1);
    if (!scriptContent) {
        lua_close(L);
        memcpy(vram, vram_bak, sizeof(*vram_bak));
        LCD_Refresh();
        free(vram_bak);
        return false;
    }

    fread(scriptContent, 1, file_size, file.get());
    scriptContent[file_size] = '\0';

    // 4. Run Script
    if (luaL_dostring(L, scriptContent)) {
        // Output error to screen
        const char* err = lua_tostring(L, -1);
        Debug_Printf(0, 0, false, 0, "%s", err);
        LCD_Refresh();
    }

    // Wait for user to dismiss
    Debug_WaitKey();

    // 5. Cleanup
    free(scriptContent);
    lua_close(L);

    // Restore Screen
    memcpy(vram, vram_bak, sizeof(*vram_bak));
    LCD_Refresh();
    free(vram_bak);

    return true;
}
