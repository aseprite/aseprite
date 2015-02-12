// Aseprite Base Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_DEBUG_H_INCLUDED
#define BASE_DEBUG_H_INCLUDED
#pragma once

int base_assert(const char* condition, const char* file, int lineNum);
void base_trace(const char* msg, ...);

#undef ASSERT
#undef TRACE

#ifdef _DEBUG
  #ifdef _WIN32
    #include <crtdbg.h>
    #define base_break() _CrtDbgBreak()
  #else
    #include <signal.h>
    #define base_break() raise(SIGTRAP)
  #endif

  #define ASSERT(condition) {                                             \
    if (!(condition)) {                                                   \
      if (base_assert(#condition, __FILE__, __LINE__)) {                  \
        base_break();                                                     \
      }                                                                   \
    }                                                                     \
  }

  #define TRACE base_trace
#else
  #define ASSERT(condition)
  #define TRACE(...)
#endif

#endif
