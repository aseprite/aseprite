// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_LUACPP_H_INCLUDED
#define APP_SCRIPT_LUACPP_H_INCLUDED
#pragma once

extern "C" {
  #include "lua.h"
  #include "lualib.h"
  #include "lauxlib.h"
}

#include "base/debug.h"

#include <functional>
#include <type_traits>

namespace app {
namespace script {

// Some of these auxiliary methods are based on code from the Skia
// library (SkLua.cpp file) by Google Inc.

template <typename T> const char* get_mtname();
#define DEF_MTNAME(T)                         \
  template <> const char* get_mtname<T>() {   \
    return #T;                                \
  }

#define DEF_MTNAME_ALIAS(T, ALIAS)              \
  template <> const char* get_mtname<ALIAS>() { \
    return #T;                                  \
  }

template <typename T, typename... Args> T* push_new(lua_State* L, Args&&... args) {
  T* addr = (T*)lua_newuserdata(L, sizeof(T));
  new (addr) T(std::forward<Args>(args)...);
  luaL_getmetatable(L, get_mtname<T>());
  lua_setmetatable(L, -2);
  return addr;
}

template <typename T> void push_obj(lua_State* L, const T& obj) {
  new (lua_newuserdata(L, sizeof(T))) T(obj);
  luaL_getmetatable(L, get_mtname<T>());
  lua_setmetatable(L, -2);
}

template <typename T> T* push_ptr(lua_State* L, T* ptr) {
  *(T**)lua_newuserdata(L, sizeof(T*)) = ptr;
  luaL_getmetatable(L, get_mtname<T>());
  lua_setmetatable(L, -2);
  return ptr;
}

template <typename T> T* get_ptr(lua_State* L, int index) {
  return *(T**)luaL_checkudata(L, index, get_mtname<T>());
}

template <typename T> T* get_obj(lua_State* L, int index) {
  T* ptr = (T*)luaL_checkudata(L, index, get_mtname<T>());
  ASSERT(typeid(*ptr) == typeid(T));
  return ptr;
}

// Returns nil if the index doesn't have the given metatable
template <typename T> T* may_get_ptr(lua_State* L, int index) {
  T** ptr = (T**)luaL_testudata(L, index, get_mtname<T>());
  if (ptr) {
    ASSERT(typeid(**ptr) == typeid(T));
    return *ptr;
  }
  else
    return nullptr;
}

// Returns nil if the index doesn't have the given metatable
template <typename T> T* may_get_obj(lua_State* L, int index) {
  return (T*)luaL_testudata(L, index, get_mtname<T>());
}

inline bool lua2bool(lua_State* L, int index) {
  return !!lua_toboolean(L, index);
}

template<typename T>
inline void setfield_integer(lua_State* L, const char* key, const T& value) {
  lua_pushinteger(L, int(value));
  lua_setfield(L, -2, key);
}

#define REG_CLASS(L, T) {                        \
    luaL_newmetatable(L, get_mtname<T>());       \
    lua_getglobal(L, "__generic_mt_index");      \
    lua_setfield(L, -2, "__index");              \
    lua_getglobal(L, "__generic_mt_newindex");   \
    lua_setfield(L, -2, "__newindex");           \
    luaL_setfuncs(L, T##_methods, 0);            \
    lua_pop(L, 1);                               \
  }

#define REG_CLASS_NEW(L, T) {                    \
    lua_pushcfunction(L, T##_new);               \
    lua_setglobal(L, #T);                        \
  }

struct Property {
  const char* name;
  lua_CFunction getter;
  lua_CFunction setter;
};

void run_mt_index_code(lua_State* L);
void create_mt_getters_setters(lua_State* L,
                               const char* tname,
                               const Property* properties);

#define REG_CLASS_PROPERTIES(L, T) {                                \
    luaL_getmetatable(L, get_mtname<T>());                          \
    create_mt_getters_setters(L, get_mtname<T>(), T##_properties);  \
    lua_pop(L, 1);                                                  \
  }

} // namespace script
} // namespace app

#endif
