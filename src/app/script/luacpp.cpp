// Aseprite
// Copyright (C) 2018-present  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/script/luacpp.h"

namespace app { namespace script {

int generic_mt_index(lua_State* L)
{
  if (lua_getmetatable(L, 1)) {
    lua_pushvalue(L, 2);
    lua_gettable(L, 3);
    if (!lua_isnil(L, -1))
      return 1;
    lua_pop(L, 1);

    lua_getfield(L, 3, "__getters");
    if (lua_istable(L, -1)) {
      lua_pushvalue(L, 2);
      lua_gettable(L, -2);
      if (!lua_isnil(L, -1)) {
        lua_pushvalue(L, 1);
        lua_call(L, 1, 1);
        return 1;
      }
      lua_pop(L, 1);
    }
    lua_pop(L, 1);
    lua_pop(L, 1);
  }

  if (lua_istable(L, 1)) {
    lua_pushvalue(L, 2);
    lua_rawget(L, 1);
    return 1;
  }

  lua_getglobal(L, "debug");
  lua_getfield(L, -1, "traceback");
  lua_call(L, 0, 1);
  const char* tb = lua_tostring(L, -1);

  lua_getglobal(L, "tostring");
  lua_pushvalue(L, 2);
  lua_call(L, 1, 1);
  const char* k = lua_tostring(L, -1);

  return luaL_error(L, "%s: Field %s does not exist", tb ? tb : "", k ? k : "nil");
}

int generic_mt_newindex(lua_State* L)
{
  if (lua_getmetatable(L, 1)) {
    lua_pushvalue(L, 2);
    lua_gettable(L, 4);
    if (!lua_isnil(L, -1))
      return 1;
    lua_pop(L, 1);

    lua_getfield(L, 4, "__setters");
    if (lua_istable(L, -1)) {
      lua_pushvalue(L, 2);
      lua_gettable(L, -2);
      if (!lua_isnil(L, -1)) {
        lua_pushvalue(L, 1);
        lua_pushvalue(L, 3);
        lua_call(L, 2, 1);
        return 1;
      }
      lua_pop(L, 1);
    }
    lua_pop(L, 1);
    lua_pop(L, 1);
  }

  if (lua_istable(L, 1)) {
    lua_pushvalue(L, 2);
    lua_pushvalue(L, 3);
    lua_rawset(L, 1);
    return 1;
  }

  lua_getglobal(L, "debug");
  lua_getfield(L, -1, "traceback");
  lua_call(L, 0, 1);
  const char* tb = lua_tostring(L, -1);

  lua_getglobal(L, "tostring");
  lua_pushvalue(L, 2);
  lua_call(L, 1, 1);
  const char* k = lua_tostring(L, -1);

  return luaL_error(L, "%s: Cannot set field %s", tb ? tb : "", k ? k : "nil");
}

void create_mt_getters_setters(lua_State* L, const char* tname, const Property* properties)
{
#ifdef _DEBUG
  const int top = lua_gettop(L);
#endif

  int numGetters = 0;
  int numSetters = 0;

  for (auto p = properties; p->name; ++p) {
    if (p->getter)
      numGetters++;
    if (p->setter)
      numSetters++;
  }
  ASSERT(numGetters > 0 || numSetters > 0);

  luaL_getmetatable(L, tname);
  if (numGetters > 0) {
    lua_createtable(L, 0, numGetters);

    for (const auto* p = properties; p->name; ++p) {
      if (p->getter) {
        lua_pushcclosure(L, p->getter, 0);
        lua_setfield(L, -2, p->name);
      }
    }
    lua_setfield(L, -2, "__getters");
  }

  if (numSetters > 0) {
    lua_createtable(L, 0, numSetters);

    for (const auto* p = properties; p->name; ++p) {
      if (p->setter) {
        lua_pushcclosure(L, p->setter, 0);
        lua_setfield(L, -2, p->name);
      }
    }
    lua_setfield(L, -2, "__setters");
  }
  lua_pop(L, 1);

  ASSERT(lua_gettop(L) == top);
}

bool lua_is_key_true(lua_State* L, int tableIndex, const char* keyName)
{
  bool result = false;
  int type = lua_getfield(L, tableIndex, keyName);
  if (type != LUA_TNIL && lua_toboolean(L, -1))
    result = true;
  lua_pop(L, 1);
  return result;
}

}} // namespace app::script
