#ifndef CFFI_H
#define CFFI_H

#include <lua.h>

// rename ffi_module_open from cffi-lua to allow including it without its headers, whose overloads 
// of global new and delete operators may cause conflicts (i.e. in msvc)
void register_ffi_object(lua_State *L);

#endif