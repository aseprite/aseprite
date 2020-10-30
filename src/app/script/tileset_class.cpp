// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "doc/tileset.h"

namespace app {
namespace script {

using namespace doc;

namespace {

int Tileset_eq(lua_State* L)
{
  const auto a = get_docobj<Tileset>(L, 1);
  const auto b = get_docobj<Tileset>(L, 2);
  lua_pushboolean(L, a->id() == b->id());
  return 1;
}

int Tileset_len(lua_State* L)
{
  auto tileset = get_docobj<Tileset>(L, 1);
  lua_pushinteger(L, tileset->size());
  return 1;
}

int Tileset_getTile(lua_State* L)
{
  auto tileset = get_docobj<Tileset>(L, 1);
  tile_index i = lua_tointeger(L, 2);
  ImageRef image = tileset->get(i);
  if (image)
    push_tileset_image(L, tileset, image.get());
  else
    lua_pushnil(L);
  return 1;
}

int Tileset_get_name(lua_State* L)
{
  auto tileset = get_docobj<Tileset>(L, 1);
  lua_pushstring(L, tileset->name().c_str());
  return 1;
}

int Tileset_set_name(lua_State* L)
{
  auto tileset = get_docobj<Tileset>(L, 1);
  if (const char* newName = lua_tostring(L, 2))
    tileset->setName(newName);
  return 0;
}

int Tileset_get_grid(lua_State* L)
{
  auto tileset = get_docobj<Tileset>(L, 1);
  push_obj(L, tileset->grid());
  return 1;
}

int Tileset_get_baseIndex(lua_State* L)
{
  auto tileset = get_docobj<Tileset>(L, 1);
  lua_pushinteger(L, tileset->baseIndex());
  return 1;
}

int Tileset_set_baseIndex(lua_State* L)
{
  auto tileset = get_docobj<Tileset>(L, 1);
  int i = lua_tointeger(L, 2);
  tileset->setBaseIndex(i);
  return 0;
}

const luaL_Reg Tileset_methods[] = {
  { "__eq", Tileset_eq },
  { "__len", Tileset_len },
  { "getTile", Tileset_getTile },
  // TODO
  // { "setTile", Tileset_setTile },
  { nullptr, nullptr }
};

const Property Tileset_properties[] = {
  { "name", Tileset_get_name, Tileset_set_name },
  { "grid", Tileset_get_grid, nullptr },
  { "baseIndex", Tileset_get_baseIndex, Tileset_set_baseIndex },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(Tileset);

void register_tileset_class(lua_State* L)
{
  using Tileset = doc::Tileset;
  REG_CLASS(L, Tileset);
  REG_CLASS_PROPERTIES(L, Tileset);
}

void push_tileset(lua_State* L, Tileset* tileset)
{
  push_docobj(L, tileset);
}

} // namespace script
} // namespace app
