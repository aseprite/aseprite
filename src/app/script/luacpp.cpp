// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/luacpp.h"

#include <cstdio>

namespace app {
namespace script {

static const char mt_index_code[] =
  "__generic_mt_index = function(t, k) "
  "  local mt = getmetatable(t) "
  "  local f = mt[k] "
  "  if f then return f end "
  "  f = mt.__getters[k] "
  "  if f then return f(t) end "
  "  if type(t) == 'table' then return rawget(t, k) end "
  "  error(debug.traceback()..': Field '..tostring(k)..' does not exist')"
  "end "
  "__generic_mt_newindex = function(t, k, v) "
  "  local mt = getmetatable(t) "
  "  local f = mt[k] "
  "  if f then return f end "
  "  f = mt.__setters[k] "
  "  if f then return f(t, v) end "
  "  if type(t) == 'table' then return rawset(t, k, v) end "
  "  error(debug.traceback()..': Cannot set field '..tostring(k))"
  "end";

void run_mt_index_code(lua_State* L)
{
  if (luaL_loadbuffer(L, mt_index_code, sizeof(mt_index_code)-1, "internal") ||
      lua_pcall(L, 0, 0, 0)) {
    // Error case
    const char* s = lua_tostring(L, -1);
    if (s)
      std::puts(s);

    lua_pop(L, 1);
  }
}

void create_mt_getters_setters(lua_State* L,
                               const char* tname,
                               const Property* properties)
{
#ifdef _DEBUG
  const int top = lua_gettop(L);
#endif

  bool withGetters = false;
  bool withSetters = false;
  for (auto p=properties; p->name; ++p) {
    if (p->getter) withGetters = true;
    if (p->setter) withSetters = true;
  }
  ASSERT(withGetters || withSetters);

  luaL_getmetatable(L, tname);
  if (withGetters) {
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setfield(L, -3, "__getters");
    for (auto p=properties; p->name; ++p) {
      if (p->getter) {
        lua_pushcclosure(L, p->getter, 0);
        lua_setfield(L, -2, p->name);
      }
    }
    lua_pop(L, 1);
  }
  if (withSetters) {
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setfield(L, -3, "__setters");
    for (auto p=properties; p->name; ++p) {
      if (p->setter) {
        lua_pushcclosure(L, p->setter, 0);
        lua_setfield(L, -2, p->name);
      }
    }
    lua_pop(L, 1);
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

} // namespace script
} // namespace app
