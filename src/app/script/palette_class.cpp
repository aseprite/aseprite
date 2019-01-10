// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_palette.h"
#include "app/color.h"
#include "app/script/docobj.h"
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
  ObjectId spriteId;
  ObjectId paletteId;

  PaletteObj(Sprite* sprite, Palette* palette)
    : spriteId(sprite ? sprite->id(): 0),
      paletteId(palette ? palette->id(): 0) {
  }

  ~PaletteObj() {
    ASSERT(!paletteId);
  }

  void gc(lua_State* L) {
    if (!spriteId)
      delete this->palette(L);
    paletteId = 0;
  }

  PaletteObj(const PaletteObj&) = delete;
  PaletteObj& operator=(const PaletteObj&) = delete;

  Sprite* sprite(lua_State* L) {
    if (spriteId)
      return check_docobj(L, doc::get<Sprite>(spriteId));
    else
      return nullptr;
  }

  Palette* palette(lua_State* L) {
    return check_docobj(L, doc::get<Palette>(paletteId));
  }
};

int Palette_new(lua_State* L)
{
  if (auto pal2 = may_get_obj<PaletteObj>(L, 1)) {
    push_new<PaletteObj>(L, nullptr, new Palette(*pal2->palette(L)));
  }
  else {
    int ncolors = lua_tointeger(L, 1);
    push_new<PaletteObj>(L, nullptr,
                         new Palette(0, ncolors > 0 ? ncolors: 256));
  }
  return 1;
}

int Palette_gc(lua_State* L)
{
  auto obj = get_obj<PaletteObj>(L, 1);
  obj->gc(L);
  obj->~PaletteObj();
  return 0;
}

int Palette_len(lua_State* L)
{
  auto obj = get_obj<PaletteObj>(L, 1);
  auto pal = obj->palette(L);
  lua_pushinteger(L, pal->size());
  return 1;
}

int Palette_resize(lua_State* L)
{
  auto obj = get_obj<PaletteObj>(L, 1);
  auto pal = obj->palette(L);
  int ncolors = lua_tointeger(L, 2);
  if (auto sprite = obj->sprite(L)) {
    Palette newPal(*pal);
    newPal.resize(ncolors);

    if (*pal != newPal) {
      Tx tx;
      tx(new cmd::SetPalette(sprite, pal->frame(), &newPal));
      tx.commit();
    }
  }
  else
    pal->resize(ncolors);
  return 1;
}

int Palette_getColor(lua_State* L)
{
  auto obj = get_obj<PaletteObj>(L, 1);
  auto pal = obj->palette(L);
  int i = lua_tointeger(L, 2);
  if (i < 0 || i >= int(pal->size()))
    return luaL_error(L, "index out of bounds %d", i);

  doc::color_t docColor = pal->getEntry(i);
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
  auto pal = obj->palette(L);
  int i = lua_tointeger(L, 2);
  if (i < 0 || i >= int(pal->size()))
    return luaL_error(L, "index out of bounds %d", i);

  doc::color_t docColor = convert_args_into_pixel_color(L, 3);

  if (auto sprite = obj->sprite(L)) {
    // Nothing to do
    if (pal->getEntry(i) == docColor)
      return 0;

    Palette newPal(*pal);
    newPal.setEntry(i, docColor);

    Tx tx;
    tx(new cmd::SetPalette(sprite, pal->frame(), &newPal));
    tx.commit();
  }
  else {
    pal->setEntry(i, docColor);
  }
  return 0;
}

int Palette_get_frame(lua_State* L)
{
  auto obj = get_obj<PaletteObj>(L, 1);
  auto pal = obj->palette(L);
  if (auto sprite = obj->sprite(L))
    push_sprite_frame(L, sprite, pal->frame());
  else
    lua_pushnil(L);
  return 1;
}

int Palette_get_frameNumber(lua_State* L)
{
  auto obj = get_obj<PaletteObj>(L, 1);
  auto pal = obj->palette(L);
  lua_pushinteger(L, pal->frame()+1);
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
  { "frameNumber", Palette_get_frameNumber, nullptr },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(PaletteObj);
DEF_MTNAME_ALIAS(PaletteObj, Palette);

void register_palette_class(lua_State* L)
{
  using Palette = PaletteObj;
  REG_CLASS(L, Palette);
  REG_CLASS_NEW(L, Palette);
  REG_CLASS_PROPERTIES(L, Palette);
}

void push_sprite_palette(lua_State* L, doc::Sprite* sprite, doc::Palette* palette)
{
  ASSERT(sprite);
  push_new<PaletteObj>(L, sprite, palette);
}

doc::Palette* get_palette_from_arg(lua_State* L, int index)
{
  auto obj = get_obj<PaletteObj>(L, index);
  return obj->palette(L);
}

} // namespace script
} // namespace app
