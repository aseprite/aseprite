// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_palette.h"
#include "app/color.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/tx.h"
#include "doc/palette.h"
#include "doc/sprite.h"

namespace app {
namespace script {

using namespace doc;

namespace {

struct PaletteObj {
  Sprite* sprite;
  Palette* palette;
  PaletteObj(Sprite* sprite, Palette* palette)
    : sprite(sprite),
      palette(palette) {
  }
  PaletteObj(const PaletteObj&) = delete;
  PaletteObj& operator=(const PaletteObj&) = delete;
};

int Palette_new(lua_State* L)
{
  int ncolors = lua_tointeger(L, 1);
  push_new<PaletteObj>(L, nullptr, new Palette(0, ncolors));
  return 1;
}

int Palette_gc(lua_State* L)
{
  get_obj<PaletteObj>(L, 1)->~PaletteObj();
  return 0;
}

int Palette_len(lua_State* L)
{
  auto obj = get_obj<PaletteObj>(L, 1);
  lua_pushinteger(L, obj->palette->size());
  return 1;
}

int Palette_resize(lua_State* L)
{
  auto obj = get_obj<PaletteObj>(L, 1);
  int ncolors = lua_tointeger(L, 2);
  if (obj->sprite) {
    Palette newPal(*obj->palette);
    newPal.resize(ncolors);

    Tx tx;
    tx(new cmd::SetPalette(obj->sprite, obj->palette->frame(), &newPal));
    tx.commit();
  }
  else
    obj->palette->resize(ncolors);
  return 1;
}

int Palette_getColor(lua_State* L)
{
  auto obj = get_obj<PaletteObj>(L, 1);
  int i = lua_tointeger(L, 2);
  if (i < 0 || i >= int(obj->palette->size()))
    return luaL_error(L, "index out of bounds %d", i);

  doc::color_t docColor = obj->palette->getEntry(i);
  app::Color appColor = app::Color::fromRgb(doc::rgba_getr(docColor),
                                            doc::rgba_getg(docColor),
                                            doc::rgba_getb(docColor),
                                            doc::rgba_geta(docColor));

  push_obj<app::Color>(L, appColor);
  return 1;
}

int Palette_setColor(lua_State* L)
{
  auto obj = get_obj<PaletteObj>(L, 1);
  int i = lua_tointeger(L, 2);
  if (i < 0 || i >= int(obj->palette->size()))
    return luaL_error(L, "index out of bounds %d", i);

  doc::color_t docColor = convert_args_into_pixel_color(L, 3);

  if (obj->sprite) {
    Palette newPal(*obj->palette);
    newPal.setEntry(i, docColor);

    Tx tx;
    tx(new cmd::SetPalette(obj->sprite, obj->palette->frame(), &newPal));
    tx.commit();
  }
  else {
    obj->palette->setEntry(i, docColor);
  }
  return 0;
}

int Palette_get_frame(lua_State* L)
{
  auto obj = get_obj<PaletteObj>(L, 1);
  lua_pushinteger(L, obj->palette->frame()+1);
  return 1;
}

const luaL_Reg Palette_methods[] = {
  { "__gc", Palette_gc },
  { "__len", Palette_len },
  { "resize", Palette_resize },
  { "getColor", Palette_getColor },
  { "setColor", Palette_setColor },
  { nullptr, nullptr }
};

const Property Palette_properties[] = {
  { "frame", Palette_get_frame, nullptr },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(PaletteObj);

void register_palette_class(lua_State* L)
{
  using Palette = PaletteObj;
  REG_CLASS(L, Palette);
  REG_CLASS_NEW(L, Palette);
  REG_CLASS_PROPERTIES(L, Palette);
}

void push_sprite_palette(lua_State* L, doc::Sprite* sprite, doc::Palette* palette)
{
  push_new<PaletteObj>(L, sprite, palette);
}

} // namespace script
} // namespace app
