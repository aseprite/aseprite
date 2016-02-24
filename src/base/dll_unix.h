// Aseprite Base Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "base/string.h"
#include <dlfcn.h>

namespace base {

dll load_dll(const std::string& filename)
{
  return dlopen(filename.c_str(), RTLD_LAZY | RTLD_GLOBAL);
}

void unload_dll(dll lib)
{
  dlclose(lib);
}

dll_proc get_dll_proc_base(dll lib, const char* procName)
{
  return dlsym(lib, procName);
}

} // namespace base
