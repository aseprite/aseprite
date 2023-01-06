// Aseprite
// Copyright (C) 2022-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_tile_data_properties.h"
#include "app/cmd/set_tile_data_property.h"
#include "app/cmd/set_user_data_properties.h"
#include "app/cmd/set_user_data_property.h"
#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/script/values.h"
#include "app/tx.h"
#include "doc/tileset.h"
#include "doc/with_user_data.h"

#include <cstring>

namespace app {
namespace script {

namespace {

struct Properties {
  // WithUserData ID or tileset ID
  doc::ObjectId id = 0;
  // If notile, it's the properties of a WithUserData, in other case,
  // it's the user data of this specific tile.
  doc::tile_index ti = doc::notile;
  std::string extID;

  Properties(const doc::WithUserData* wud,
             const std::string& extID)
    : id(wud->id())
    , extID(extID) {
  }

  Properties(const doc::Tileset* ts,
             const doc::tile_index ti,
             const std::string& extID)
    : id(ts->id())
    , ti(ti)
    , extID(extID) {
  }

  Properties(const Properties& copy,
             const std::string& extID)
    : id(copy.id)
    , ti(copy.ti)
    , extID(extID) {
  }

  doc::WithUserData* object(lua_State* L) {
    auto obj = static_cast<doc::WithUserData*>(doc::get_object(id));
    if (!obj) {
      // luaL_error never returns
      luaL_error(L, "the object with these properties was destroyed");
    }
    return obj;
  }

  doc::UserData::Properties& properties(lua_State* L, doc::WithUserData* obj = nullptr) {
    if (!obj)
      obj = object(L);
    if (ti == doc::notile) {
      return obj->userData().properties(extID);
    }
    else {
      ASSERT(obj->type() == doc::ObjectType::Tileset);
      return
        const_cast<doc::UserData*>(
          &static_cast<doc::Tileset*>(obj)->getTileData(ti))
        ->properties(extID);
    }
  }

};

using PropertiesIterator = doc::UserData::Properties::iterator;

int Properties_len(lua_State* L)
{
  auto propObj = get_obj<Properties>(L, 1);
  auto& properties = propObj->properties(L);
  lua_pushinteger(L, properties.size());
  return 1;
}

int Properties_index(lua_State* L)
{
  auto propObj = get_obj<Properties>(L, 1);
  const char* field = lua_tostring(L, 2);
  if (!field)
    return luaL_error(L, "field in 'properties.field' must be a string");

  auto& properties = propObj->properties(L);
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

  auto& properties = propObj->properties(L, obj);
  auto newValue = get_value_from_lua<doc::UserData::Variant>(L, 3);

  // TODO add Object::sprite() member function
  //if (obj->sprite()) {
  if (App::instance()->context()->activeDocument()) {
    Tx tx;
    if (propObj->ti != doc::notile) {
      tx(new cmd::SetTileDataProperty(static_cast<doc::Tileset*>(obj),
                                      propObj->ti, propObj->extID, field,
                                      std::move(newValue)));
    }
    else {
      tx(new cmd::SetUserDataProperty(obj, propObj->extID, field,
                                      std::move(newValue)));
    }
    tx.commit();
  }
  else {
    properties[field] = std::move(newValue);
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
      if (propObj->ti != doc::notile) {
        tx(new cmd::SetTileDataProperties(static_cast<doc::Tileset*>(obj),
                                          propObj->ti, extID, std::move(newProperties)));
      }
      else {
        tx(new cmd::SetUserDataProperties(obj, extID, std::move(newProperties)));
      }
      tx.commit();
    }
    else {
      auto& properties = obj->userData().properties(extID);
      properties = std::move(newProperties);
    }
  }

  push_new<Properties>(L, *propObj, extID);
  return 1;
}

int Properties_pairs_next(lua_State* L)
{
  auto propObj = get_obj<Properties>(L, 1);
  auto& properties = propObj->properties(L);
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

  auto& properties = propObj->properties(L);

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
                     doc::WithUserData* wud,
                     const std::string& extID)
{
  push_new<Properties>(L, wud, extID);
}

void push_tile_properties(lua_State* L,
                          const doc::Tileset* ts,
                          doc::tile_index ti,
                          const std::string& extID)
{
  push_new<Properties>(L, ts, ti, extID);
}

} // namespace script
} // namespace app
