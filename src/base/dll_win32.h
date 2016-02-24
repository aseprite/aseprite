// Aseprite Base Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "base/string.h"
#include <windows.h>

namespace base {

dll load_dll(const std::string& filename)
{
  return LoadLibrary(base::from_utf8(filename).c_str());
}

void unload_dll(dll lib)
{
  FreeLibrary((HMODULE)lib);
}

dll_proc get_dll_proc_base(dll lib, const char* procName)
{
  return reinterpret_cast<dll_proc>(
    GetProcAddress((HMODULE)lib, procName));
}


} // namespace base
