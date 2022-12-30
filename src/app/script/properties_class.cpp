// Aseprite
// Copyright (C) 2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/script/values.h"
#include "doc/with_user_data.h"

#include <cstring>

namespace app {
namespace script {

namespace {

struct Properties {
  doc::ObjectId id = 0;
  Properties(doc::ObjectId id) : id(id) { }
};

int Properties_index(lua_State* L)
{
  auto propObj = get_obj<Properties>(L, 1);
  const char* field = lua_tostring(L, 2);
  if (!field)
    return luaL_error(L, "field in 'properties.field' must be a string");

  auto obj = static_cast<doc::WithUserData*>(get_object(propObj->id));
  if (!obj)
    return luaL_error(L, "the object with these properties was destroyed");

  auto& properties = obj->userData().properties();
  auto it = properties.find(field);
  if (it != properties.end()) {
    push_value_to_lua(L, (*it).second);
  }
  else {
    lua_pushnil(L);
  }
  return 1;
}

int Properties_newindex(lua_State* L)
{
  auto propObj = get_obj<Properties>(L, 1);
  const char* field = lua_tostring(L, 2);
  if (!field)
    return luaL_error(L, "field in 'properties.field' must be a string");

  auto obj = static_cast<doc::WithUserData*>(get_object(propObj->id));
  if (!obj)
    return luaL_error(L, "the object with these properties was destroyed");

  auto& properties = obj->userData().properties();

  // TODO add undo information
  switch (lua_type(L, 3)) {

    case LUA_TNONE:
    case LUA_TNIL:
    default: {
      // Just erase the property
      auto it = properties.find(field);
      if (it != properties.end())
        properties.erase(it);
      break;
    }

    case LUA_TBOOLEAN:
      properties[field] = (lua_toboolean(L, 3) ? true: false);
      break;

    case LUA_TNUMBER:
      if (lua_isinteger(L, 3))
        properties[field] = lua_tointeger(L, 3);
      else {
        properties[field] = doc::UserData::Fixed{
          fixmath::ftofix(lua_tonumber(L, 3))
        };
      }
      break;

    case LUA_TSTRING:
      properties[field] = lua_tostring(L, 3);
      break;

    case LUA_TTABLE:
      // TODO convert a full table into properties recursively
      break;

    case LUA_TUSERDATA:
      // TODO convert table-like objects (Size, Point, Rectangle, etc.)
      break;

  }
  return 1;
}

const luaL_Reg Properties_methods[] = {
  { "__index", Properties_index },
  { "__newindex", Properties_newindex },
  { nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(Properties);

void register_properties_class(lua_State* L)
{
  REG_CLASS(L, Properties);
}

void push_properties(lua_State* L, doc::WithUserData* userData)
{
  push_obj<Properties>(L, userData->id());
}

} // namespace script
} // namespace app
