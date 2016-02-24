// Aseprite Base Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_DLL_H_INCLUDED
#define BASE_DLL_H_INCLUDED
#pragma once

#include <string>

namespace base {

typedef void* dll;
typedef void* dll_proc;

dll load_dll(const std::string& filename);
void unload_dll(dll lib);
dll_proc get_dll_proc_base(dll lib, const char* procName);

template<typename T>
inline T get_dll_proc(dll lib, const char* procName) {
  return reinterpret_cast<T>(get_dll_proc_base(lib, procName));
}

} // namespace base

#endif
