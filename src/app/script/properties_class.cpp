// Aseprite
// Copyright (C) 2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/engine.h"
#include "app/script/luacpp.h"

#include <string>

namespace app {
namespace script {

namespace {

using namespace doc;

struct PropertiesMapsObj {
  PropertiesMaps propsMaps;
  PropertiesMapsObj(const PropertiesMaps& propertiesMaps)
    : propsMaps(propertiesMaps) {
  }
};

struct PropertiesObj {
  Properties props;
  PropertiesObj(const Properties& properties)
    : props(properties) {
  }
};

struct VariantStructObj {
  VariantStruct variant;
  VariantStructObj(const VariantStruct& variant)
    : variant(variant) {
  }
};

int PropertiesMaps_index(lua_State* L) {
  auto props = get_obj<PropertiesMapsObj>(L, 1);

  const char* id = lua_tostring(L, 2);
  if (!id)
    return luaL_error(L, "id in '...PropertiesMaps' must be a string");

  push_properties(L, props->propsMaps.at(id));
  return 1;
}

int PropertiesMaps_newindex(lua_State* L)
{
  return 0;
}

int Properties_index(lua_State* L) {
  auto props = get_obj<PropertiesObj>(L, 1);
  const char* id = lua_tostring(L, 2);
  if (!id)
    return luaL_error(L, "id in '...properties.publisher' must be a string");
  lua_pushinteger(L, props->props.property<uint32_t>(id));
  return 1;
}

int Properties_newindex(lua_State* L)
{
  return 0;
}

int Property_index(lua_State* L) {
  auto prop = get_obj<VariantStructObj>(L, 1);

  const char* id = lua_tostring(L, 2);
  if (!id)
    return luaL_error(L, "id in '...properties.publisher' must be a string");
//  push_property(L, prop->prop);
  lua_pushnil(L);
  return 1;
}

int Property_newindex(lua_State* L)
{
  return 0;
}

const luaL_Reg PropertiesMapsObj_methods[] = {
  { "__index", PropertiesMaps_index },
  { "__newindex", PropertiesMaps_newindex },
  { nullptr, nullptr }
};

const luaL_Reg PropertiesObj_methods[] = {
  { "__index", Properties_index },
  { "__newindex", Properties_newindex },
  { nullptr, nullptr }
};

const luaL_Reg VariantStructObj_methods[] = {
  { "__index", Property_index },
  { "__newindex", Property_newindex },
  { nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(PropertiesMapsObj);
DEF_MTNAME(PropertiesObj);
DEF_MTNAME(VariantStructObj);

void register_properties_maps_class(lua_State* L)
{
  REG_CLASS(L, PropertiesMapsObj);
}

void push_properties_maps(lua_State* L, const PropertiesMaps& propertiesMaps)
{
  push_new<PropertiesMapsObj>(L, propertiesMaps);
}

void register_properties_class(lua_State* L)
{
  REG_CLASS(L, PropertiesObj);
}

void push_properties(lua_State* L, const Properties& properties)
{
  push_new<PropertiesObj>(L, properties);
}

void register_property_class(lua_State* L)
{
  REG_CLASS(L, VariantStructObj);
}

void push_property(lua_State* L, const VariantStruct& property)
{
  push_new<VariantStructObj>(L, property);
}

} // namespace script
} // namespace app

