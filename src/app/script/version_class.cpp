// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/luacpp.h"
#include "base/version.h"

namespace app {
namespace script {

namespace {

base::Version Version_new(lua_State* L, int index)
{
  base::Version ver;
  if (auto ver2 = may_get_obj<base::Version>(L, index)) {
    ver = *ver2;
  }
  else if (const char* verStr = lua_tostring(L, index)) {
    ver = base::Version(verStr);
  }
  return ver;
}

int Version_new(lua_State* L)
{
  push_obj(L, Version_new(L, 1));
  return 1;
}

int Version_gc(lua_State* L)
{
  get_obj<base::Version>(L, 1)->~Version();
  return 0;
}

int Version_eq(lua_State* L)
{
  const auto a = get_obj<base::Version>(L, 1);
  const auto b = get_obj<base::Version>(L, 2);
  lua_pushboolean(L, *a == *b);
  return 1;
}

int Version_lt(lua_State* L)
{
  const auto a = get_obj<base::Version>(L, 1);
  const auto b = get_obj<base::Version>(L, 2);
  lua_pushboolean(L, *a < *b);
  return 1;
}

int Version_le(lua_State* L)
{
  const auto a = get_obj<base::Version>(L, 1);
  const auto b = get_obj<base::Version>(L, 2);
  lua_pushboolean(L, (*a < *b) || (*a == *b));
  return 1;
}

int Version_tostring(lua_State* L)
{
  const auto ver = get_obj<base::Version>(L, 1);
  lua_pushstring(L, ver->str().c_str());
  return 1;
}

int Version_get_major(lua_State* L)
{
  const auto ver = get_obj<base::Version>(L, 1);
  const auto& numbers = ver->numbers();
  lua_pushinteger(L, numbers.size() > 0 ? numbers[0]: 0);
  return 1;
}

int Version_get_minor(lua_State* L)
{
  const auto ver = get_obj<base::Version>(L, 1);
  const auto& numbers = ver->numbers();
  lua_pushinteger(L, numbers.size() > 1 ? numbers[1]: 0);
  return 1;
}

int Version_get_patch(lua_State* L)
{
  const auto ver = get_obj<base::Version>(L, 1);
  const auto& numbers = ver->numbers();
  lua_pushinteger(L, numbers.size() > 2 ? numbers[2]: 0);
  return 1;
}

int Version_get_prereleaseLabel(lua_State* L)
{
  const auto ver = get_obj<base::Version>(L, 1);
  lua_pushstring(L, ver->prereleaseLabel().c_str());
  return 1;
}

int Version_get_prereleaseNumber(lua_State* L)
{
  const auto ver = get_obj<base::Version>(L, 1);
  lua_pushinteger(L, ver->prereleaseNumber());
  return 1;
}

const luaL_Reg Version_methods[] = {
  { "__gc", Version_gc },
  { "__eq", Version_eq },
  { "__lt", Version_lt },
  { "__le", Version_le },
  { "__tostring", Version_tostring },
  { nullptr, nullptr }
};

const Property Version_properties[] = {
  { "major", Version_get_major, nullptr },
  { "minor", Version_get_minor, nullptr },
  { "patch", Version_get_patch, nullptr },
  { "prereleaseLabel", Version_get_prereleaseLabel, nullptr },
  { "prereleaseNumber", Version_get_prereleaseNumber, nullptr },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(base::Version);

void register_version_class(lua_State* L)
{
  using base::Version;
  REG_CLASS(L, Version);
  REG_CLASS_NEW(L, Version);
  REG_CLASS_PROPERTIES(L, Version);
}

void push_version(lua_State* L, const base::Version& ver)
{
  push_obj(L, ver);
}

} // namespace script
} // namespace app
