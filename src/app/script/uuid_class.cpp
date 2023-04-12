// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "base/convert_to.h"
#include "base/uuid.h"

namespace app {
namespace script {

using Uuid = base::Uuid;

namespace {

Uuid Uuid_new(lua_State* L, int index)
{
  // Copy other uuid
  if (auto uuid = may_get_obj<Uuid>(L, index)) {
    return *uuid;
  }
  else if (const char* s = lua_tostring(L, index)) {
    return base::convert_to<Uuid>(std::string(s));
  }
  else
    return Uuid::Generate();
}

int Uuid_new(lua_State* L)
{
  push_obj(L, Uuid_new(L, 1));
  return 1;
}

int Uuid_gc(lua_State* L)
{
  get_obj<Uuid>(L, 1)->~Uuid();
  return 0;
}

int Uuid_eq(lua_State* L)
{
  const auto a = get_obj<Uuid>(L, 1);
  const auto b = get_obj<Uuid>(L, 2);
  lua_pushboolean(L, *a == *b);
  return 1;
}

int Uuid_tostring(lua_State* L)
{
  const auto uuid = get_obj<Uuid>(L, 1);
  lua_pushstring(L, base::convert_to<std::string>(*uuid).c_str());
  return 1;
}

int Uuid_index(lua_State* L)
{
  const auto uuid = get_obj<Uuid>(L, 1);
  const int i = lua_tointeger(L, 2);
  if (i >= 1 && i <= 16)
    lua_pushinteger(L, (*uuid)[i-1]);
  else
    lua_pushnil(L);
  return 1;
}

const luaL_Reg Uuid_methods[] = {
  { "__gc", Uuid_gc },
  { "__eq", Uuid_eq },
  { "__tostring", Uuid_tostring },
  { "__index", Uuid_index },
  { nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(Uuid);

void register_uuid_class(lua_State* L)
{
  REG_CLASS(L, Uuid);
  REG_CLASS_NEW(L, Uuid);
}

base::Uuid convert_args_into_uuid(lua_State* L, int index)
{
  return Uuid_new(L, index);
}

} // namespace script
} // namespace app
