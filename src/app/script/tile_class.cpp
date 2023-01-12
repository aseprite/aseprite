// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_tile_data.h"
#include "app/cmd/set_tile_data_properties.h"
#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/script/userdata.h"
#include "doc/tileset.h"

namespace app {
namespace script {

using namespace doc;

namespace {

struct Tile {
  ObjectId id;
  tile_index ti;
  Tile(const Tileset* ts,
       const tile_index ti)
    : id(ts->id())
    , ti(ti) {
  }
};

int Tile_get_index(lua_State* L)
{
  auto tile = get_obj<Tile>(L, 1);
  lua_pushinteger(L, tile->ti);
  return 1;
}

int Tile_get_image(lua_State* L)
{
  auto tile = get_obj<Tile>(L, 1);
  auto ts = doc::get<Tileset>(tile->id);
  if (!ts)
    return 0;

  ImageRef image = ts->get(tile->ti);
  if (image)
    push_tileset_image(L, ts, image.get());
  else
    lua_pushnil(L);
  return 1;
}

int Tile_get_data(lua_State* L)
{
  auto tile = get_obj<Tile>(L, 1);
  auto ts = doc::get<Tileset>(tile->id);
  if (!ts)
    return 0;

  auto& ud = ts->getTileData(tile->ti);
  lua_pushstring(L, ud.text().c_str());
  return 1;
}

int Tile_get_color(lua_State* L)
{
  auto tile = get_obj<Tile>(L, 1);
  auto ts = doc::get<Tileset>(tile->id);
  if (!ts)
    return 0;

  auto& ud = ts->getTileData(tile->ti);
  doc::color_t docColor = ud.color();
  app::Color appColor = app::Color::fromRgb(doc::rgba_getr(docColor),
                                            doc::rgba_getg(docColor),
                                            doc::rgba_getb(docColor),
                                            doc::rgba_geta(docColor));
  if (appColor.getAlpha() == 0)
    appColor = app::Color::fromMask();
  push_obj<app::Color>(L, appColor);
  return 1;
}

int Tile_get_properties(lua_State* L)
{
  auto tile = get_obj<Tile>(L, 1);
  auto ts = doc::get<Tileset>(tile->id);
  if (!ts)
    return 0;

  push_tile_properties(L, ts, tile->ti, std::string());
  return 1;
}

int Tile_set_data(lua_State* L)
{
  auto tile = get_obj<Tile>(L, 1);
  auto ts = doc::get<Tileset>(tile->id);
  if (!ts)
    return 0;

  const char* text = lua_tostring(L, 2);
  if (!text)
    text = "";

  doc::UserData ud = ts->getTileData(tile->ti);
  ud.setText(text);

  if (ts->sprite()) {           // TODO use transaction in this sprite
    Tx tx;
    tx(new cmd::SetTileData(ts, tile->ti, ud));
    tx.commit();
  }
  else {
    ts->setTileData(tile->ti, ud);
  }
  return 0;
}

int Tile_set_color(lua_State* L)
{
  auto tile = get_obj<Tile>(L, 1);
  auto ts = doc::get<Tileset>(tile->id);
  if (!ts)
    return 0;

  doc::color_t docColor = convert_args_into_pixel_color(L, 2, doc::IMAGE_RGB);

  doc::UserData ud = ts->getTileData(tile->ti);
  ud.setColor(docColor);

  if (ts->sprite()) {           // TODO use transaction in this sprite
    Tx tx;
    tx(new cmd::SetTileData(ts, tile->ti, ud));
    tx.commit();
  }
  else {
    ts->setTileData(tile->ti, ud);
  }
  return 0;
}

int Tile_set_properties(lua_State* L)
{
  auto tile = get_obj<Tile>(L, 1);
  auto ts = doc::get<Tileset>(tile->id);
  if (!ts)
    return 0;

  auto newProperties = get_value_from_lua<doc::UserData::Properties>(L, 2);
  if (ts->sprite()) {
    Tx tx;
    tx(new cmd::SetTileDataProperties(ts, tile->ti,
                                      std::string(),
                                      std::move(newProperties)));
    tx.commit();
  }
  else {
    auto& properties = ts->getTileData(tile->ti).properties();
    properties = std::move(newProperties);
  }
  return 0;
}

const luaL_Reg Tile_methods[] = {
  { nullptr, nullptr }
};

const Property Tile_properties[] = {
  { "index", Tile_get_index, nullptr },
  { "image", Tile_get_image, nullptr }, // TODO Tile_set_image
  { "data", Tile_get_data, Tile_set_data },
  { "color", Tile_get_color, Tile_set_color },
  { "properties", Tile_get_properties, Tile_set_properties },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(Tile);

void register_tile_class(lua_State* L)
{
  REG_CLASS(L, Tile);
  REG_CLASS_PROPERTIES(L, Tile);
}

void push_tile(lua_State* L, const Tileset* ts, tile_index ti)
{
  push_new<Tile>(L, ts, ti);
}

Tileset* get_tile_index_from_arg(lua_State* L, int index, tile_index& ti)
{
  Tile* tile = get_obj<Tile>(L, index);
  ti = tile->ti;
  return doc::get<Tileset>(tile->id);
}

} // namespace script
} // namespace app
