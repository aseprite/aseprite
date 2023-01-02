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

using PropertiesIterator = doc::UserData::Properties::iterator;

int Properties_len(lua_State* L)
{
  auto propObj = get_obj<Properties>(L, 1);
  auto obj = static_cast<doc::WithUserData*>(get_object(propObj->id));
  if (!obj)
    return luaL_error(L, "the object with these properties was destroyed");

  auto& properties = obj->userData().properties();
  lua_pushinteger(L, properties.size());
  return 1;
}

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
      properties[field] = std::string(lua_tostring(L, 3));
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

int Properties_pairs_next(lua_State* L)
{
  auto propObj = get_obj<Properties>(L, 1);
  auto obj = static_cast<doc::WithUserData*>(get_object(propObj->id));
  if (!obj)
    return luaL_error(L, "the object with these properties was destroyed");

  auto& properties = obj->userData().properties();
  auto& it = *get_obj<PropertiesIterator>(L, lua_upvalueindex(1));
  if (it == properties.end())
    return 0;
  lua_pushstring(L, (*it).first.c_str());
  push_value_to_lua(L, (*it).second);
  ++it;
  return 2;
}

int Properties_pairs(lua_State* L)
{
  auto propObj = get_obj<Properties>(L, 1);
  auto obj = static_cast<doc::WithUserData*>(get_object(propObj->id));
  if (!obj)
    return luaL_error(L, "the object with these properties was destroyed");

  auto& properties = obj->userData().properties();

  push_obj(L, properties.begin());
  lua_pushcclosure(L, Properties_pairs_next, 1);
  lua_pushvalue(L, 1); // Copy the same propObj as the second return value
  return 2;
}

int PropertiesIterator_gc(lua_State* L)
{
  get_obj<PropertiesIterator>(L, 1)->~PropertiesIterator();
  return 0;
}

const luaL_Reg Properties_methods[] = {
  { "__len", Properties_len },
  { "__index", Properties_index },
  { "__newindex", Properties_newindex },
  { "__pairs", Properties_pairs },
  { nullptr, nullptr }
};

const luaL_Reg PropertiesIterator_methods[] = {
  { "__gc", PropertiesIterator_gc },
  { nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(Properties);
DEF_MTNAME(PropertiesIterator);

void register_properties_class(lua_State* L)
{
  REG_CLASS(L, Properties);
  REG_CLASS(L, PropertiesIterator);
}

void push_properties(lua_State* L, doc::WithUserData* userData)
{
  push_obj<Properties>(L, userData->id());
}

} // namespace script
} // namespace app
