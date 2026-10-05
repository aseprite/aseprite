#include <lua.hh>
#include <cffi.h>

void ffi_module_open(lua_State *L);

void register_ffi_object(lua_State *L) {
    ffi_module_open(L);
    lua_pushvalue(L, -1);
    lua_setglobal(L, "ffi");
    lua_pop(L, 1);
}