#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sdk/os/debug.h>

#ifdef L_tmpnam
#undef L_tmpnam
#endif
#define L_tmpnam 20

#ifdef EXIT_SUCCESS
#undef EXIT_SUCCESS
#endif
#define EXIT_SUCCESS 0

#ifdef EXIT_FAILURE
#undef EXIT_FAILURE
#endif
#define EXIT_FAILURE 1

#ifdef lua_writestring
#undef lua_writestring
#endif
#define lua_writestring(s,l) Debug_Printf(0, 0, false, 0, "%.*s", (int)(l), s)

#ifdef lua_writeline
#undef lua_writeline
#endif
#define lua_writeline() Debug_Printf(0, 0, false, 0, "\n")

#ifndef SIG_DFL
#define SIG_DFL 0
#endif

#ifndef SIGINT
#define SIGINT 1
#endif
