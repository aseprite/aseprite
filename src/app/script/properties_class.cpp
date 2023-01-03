// Aseprite
// Copyright (C) 2022-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/remove_user_data_property.h"
#include "app/cmd/set_user_data_properties.h"
#include "app/cmd/set_user_data_property.h"
#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/script/values.h"
#include "app/tx.h"
#include "doc/with_user_data.h"

#include <cstring>

namespace app {
namespace script {

namespace {

struct Properties {
  doc::ObjectId id = 0;
  std::string extID;

  Properties(doc::ObjectId id,
             const std::string& extID)
    : id(id)
    , extID(extID) {
  }
};

using PropertiesIterator = doc::UserData::Properties::iterator;

int Properties_len(lua_State* L)
{
  auto propObj = get_obj<Properties>(L, 1);
  auto obj = static_cast<doc::WithUserData*>(get_object(propObj->id));
  if (!obj)
    return luaL_error(L, "the object with these properties was destroyed");

  auto& properties = obj->userData().properties(propObj->extID);
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

  auto& properties = obj->userData().properties(propObj->extID);
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

  auto& properties = obj->userData().properties(propObj->extID);

  switch (lua_type(L, 3)) {

    case LUA_TNONE:
    case LUA_TNIL: {
      // If we assign nil to a property, we just remove the property.

      auto it = properties.find(field);
      if (it != properties.end()) {
        // TODO add Object::sprite() member function, and fix "Tx" object
        //      to use the sprite of this object instead of the activeDocument()
        //if (obj->sprite()) {
        if (App::instance()->context()->activeDocument()) {
          Tx tx;
          tx(new cmd::RemoveUserDataProperty(obj, propObj->extID, field));
          tx.commit();
        }
        else {
          properties.erase(it);
        }
      }
      break;
    }

    default: {
      auto newValue = get_value_from_lua<doc::UserData::Variant>(L, 3);

      // TODO add Object::sprite() member function
      //if (obj->sprite()) {
      if (App::instance()->context()->activeDocument()) {
        Tx tx;
        tx(new cmd::SetUserDataProperty(obj, propObj->extID, field,
                                        std::move(newValue)));
        tx.commit();
      }
      else {
        properties[field] = std::move(newValue);
      }
      break;
    }
  }
  return 0;
}

int Properties_call(lua_State* L)
{
  auto propObj = get_obj<Properties>(L, 1);
  const char* extID = lua_tostring(L, 2);
  if (!extID)
    return luaL_error(L, "extensionID in 'properties(\"extensionID\")' must be a string");

  // Special syntax to change the full extension properties using:
  //
  //   object.property("extension", { ...})
  //
  if (lua_istable(L, 3)) {
    auto newProperties = get_value_from_lua<doc::UserData::Properties>(L, 3);

    auto obj = static_cast<doc::WithUserData*>(get_object(propObj->id));
    if (!obj)
      return luaL_error(L, "the object with these properties was destroyed");

    // TODO add Object::sprite() member function
    //if (obj->sprite()) {
    if (App::instance()->context()->activeDocument()) {
      Tx tx;
      tx(new cmd::SetUserDataProperties(obj, extID, std::move(newProperties)));
      tx.commit();
    }
    else {
      auto& properties = obj->userData().properties(extID);
      properties = std::move(newProperties);
    }
  }

  push_new<Properties>(L, propObj->id, extID);
  return 1;
}

int Properties_pairs_next(lua_State* L)
{
  auto propObj = get_obj<Properties>(L, 1);
  auto obj = static_cast<doc::WithUserData*>(get_object(propObj->id));
  if (!obj)
    return luaL_error(L, "the object with these properties was destroyed");

  auto& properties = obj->userData().properties(propObj->extID);
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

  auto& properties = obj->userData().properties(propObj->extID);

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
  { "__call", Properties_call },
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

void push_properties(lua_State* L,
                     doc::WithUserData* userData,
                     const std::string& extID)
{
  push_new<Properties>(L, userData->id(), extID);
}

} // namespace script
} // namespace app
