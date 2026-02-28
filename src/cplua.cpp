#include "cplua.hpp"
#include "lua_calc.hpp"
#include <sdk/os/debug.h>
#include <sdk/os/lcd.h>
#include <sdk/os/file.h>

#include <stdlib.h>
#include <string.h>
#include <type_traits>

extern "C" {
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
}

// A backup pointer for the original vram content
std::remove_pointer_t<decltype(vram)> (*vram_bak)[LCD_WIDTH_PX * LCD_HEIGHT_PX];

// Simple RAII file handle wrapper
class SafeFileHandle {
    int fd;
public:
    SafeFileHandle(const char* filepath) {
        fd = open(filepath, OPEN_READ);
    }
    ~SafeFileHandle() {
        if (fd >= 0) close(fd);
    }
    bool is_valid() const { return fd >= 0; }
    int get() const { return fd; }
};

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
    int file_size = lseek(file.get(), 0, SEEK_END);
    lseek(file.get(), 0, SEEK_SET);

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

    read(file.get(), scriptContent, file_size);
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
