// Aseprite
// Copyright (C) 2019-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/script/luacpp.h"
#include "base/version.h"

namespace app { namespace script {

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

LUA_FUNCTION(Version_new)
{
  push_obj(L, Version_new(L, 1));
  return 1;
}

LUA_FUNCTION(Version_gc)
{
  get_obj<base::Version>(L, 1)->~Version();
  return 0;
}

LUA_FUNCTION(Version_eq)
{
  const auto a = get_obj<base::Version>(L, 1);
  const auto b = get_obj<base::Version>(L, 2);
  lua_pushboolean(L, *a == *b);
  return 1;
}

LUA_FUNCTION(Version_lt)
{
  const auto a = get_obj<base::Version>(L, 1);
  const auto b = get_obj<base::Version>(L, 2);
  lua_pushboolean(L, *a < *b);
  return 1;
}

LUA_FUNCTION(Version_le)
{
  const auto a = get_obj<base::Version>(L, 1);
  const auto b = get_obj<base::Version>(L, 2);
  lua_pushboolean(L, (*a < *b) || (*a == *b));
  return 1;
}

LUA_FUNCTION(Version_tostring)
{
  const auto ver = get_obj<base::Version>(L, 1);
  lua_pushstring(L, ver->str().c_str());
  return 1;
}

LUA_FUNCTION(Version_get_major)
{
  const auto ver = get_obj<base::Version>(L, 1);
  const auto& numbers = ver->numbers();
  lua_pushinteger(L, numbers.size() > 0 ? numbers[0] : 0);
  return 1;
}

LUA_FUNCTION(Version_get_minor)
{
  const auto ver = get_obj<base::Version>(L, 1);
  const auto& numbers = ver->numbers();
  lua_pushinteger(L, numbers.size() > 1 ? numbers[1] : 0);
  return 1;
}

LUA_FUNCTION(Version_get_patch)
{
  const auto ver = get_obj<base::Version>(L, 1);
  const auto& numbers = ver->numbers();
  lua_pushinteger(L, numbers.size() > 2 ? numbers[2] : 0);
  return 1;
}

LUA_FUNCTION(Version_get_prereleaseLabel)
{
  const auto ver = get_obj<base::Version>(L, 1);
  lua_pushstring(L, ver->prereleaseLabel().c_str());
  return 1;
}

LUA_FUNCTION(Version_get_prereleaseNumber)
{
  const auto ver = get_obj<base::Version>(L, 1);
  lua_pushinteger(L, ver->prereleaseNumber());
  return 1;
}

const luaL_Reg Version_methods[] = {
  { "__gc",       Version_gc       },
  { "__eq",       Version_eq       },
  { "__lt",       Version_lt       },
  { "__le",       Version_le       },
  { "__tostring", Version_tostring },
  { nullptr,      nullptr          }
};

const Property Version_properties[] = {
  { "major",            Version_get_major,            nullptr },
  { "minor",            Version_get_minor,            nullptr },
  { "patch",            Version_get_patch,            nullptr },
  { "prereleaseLabel",  Version_get_prereleaseLabel,  nullptr },
  { "prereleaseNumber", Version_get_prereleaseNumber, nullptr },
  { nullptr,            nullptr,                      nullptr }
};
DEF_ITERATOR_PAIRS(Version);
} // anonymous namespace

DEF_MTNAME(base::Version);

LUA_FUNCTION(register_version_class)
{
  using base::Version;
  REG_CLASS(L, Version);
  REG_CLASS_NEW(L, Version);
  REG_CLASS_PROPERTIES(L, Version);
  return 0;
}

void push_version(lua_State* L, const base::Version& ver)
{
  push_obj(L, ver);
}

}} // namespace app::script
