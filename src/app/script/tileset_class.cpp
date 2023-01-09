// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/add_tile.h"
#include "app/cmd/remove_tile.h"
#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/script/userdata.h"
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

int Tileset_tile(lua_State* L)
{
  auto tileset = get_docobj<Tileset>(L, 1);
  tile_index ti = lua_tointeger(L, 2);
  if (ti >= 0 && ti < tileset->size())
    push_tile(L, tileset, ti);
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

int Tileset_addTile(lua_State* L)
{
  auto tileset = get_docobj<Tileset>(L, 1);
  if (auto image = may_get_image_from_arg(L, 2)) {
    ImageRef tileImage(Image::createCopy(image));
    tile_index ti;
    if (tileset->findTileIndex(tileImage, ti)) {
      // Duplicated image in the tileset, do nothing.
      push_tileset(L, tileset);
      return 1;
    }
    Tx tx;
    tx(new cmd::AddTile(tileset, tileImage));
    tx.commit();
    push_tileset(L, tileset);
    return 1;
  }
  else
    return luaL_error(L, "the argument must be an image");
}

int Tileset_eraseTile(lua_State* L)
{
  auto tileset = get_docobj<Tileset>(L, 1);
  tile_index ti;
  if (auto tileImage = may_get_image_from_arg(L, 2)) {
    ImageRef imageToRemove(Image::createCopy(tileImage));
    if (!tileset->findTileIndex(imageToRemove, ti))
      return luaL_error(L, "tile image not found in tileset");
  }
  else {
    ti = tile_index(lua_tointeger(L, 2));
    if (ti <= 0)
      return luaL_error(L, "index must be equal to or greater than 1");
    else if (ti >= tileset->size())
      return luaL_error(L, "index %d is out of bounds. Max valid index is %d", ti, tileset->size()-1);
  }
  Tx tx;
  tx(new cmd::RemoveTile(tileset, ti));
  tx.commit();
  push_tileset(L, tileset);
  return 1;
}

const luaL_Reg Tileset_methods[] = {
  { "__eq", Tileset_eq },
  { "__len", Tileset_len },
  { "getTile", Tileset_getTile },
  { "tile", Tileset_tile },
  { "addTile", Tileset_addTile },
  { "eraseTile", Tileset_eraseTile },
  { nullptr, nullptr }
};

const Property Tileset_properties[] = {
  { "name", Tileset_get_name, Tileset_set_name },
  { "grid", Tileset_get_grid, nullptr },
  { "baseIndex", Tileset_get_baseIndex, Tileset_set_baseIndex },
  { "color", UserData_get_color<Tileset>, UserData_set_color<Tileset> },
  { "data", UserData_get_text<Tileset>, UserData_set_text<Tileset> },
  { "properties", UserData_get_properties<Tileset>, UserData_set_properties<Tileset> },
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

void push_tileset(lua_State* L, const Tileset* tileset)
{
  push_docobj(L, tileset);
}

} // namespace script
} // namespace app
